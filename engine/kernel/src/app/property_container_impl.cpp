// #my_engine_source_file

#include "property_container_impl.h"

#include "my/io/file_system.h"
#include "my/io/stream_utils.h"
#include "my/serialization/json.h"
#include "my/utils/string_utils.h"

namespace my
{
    namespace
    {
        // splitting the path by {parent_path, property_name}
        std::tuple<std::string_view, std::string_view> split_property_path(std::string_view propertyPath)
        {
            const auto pos = propertyPath.rfind('/');
            if (pos == std::string_view::npos)
            {
                // root path
                return {"/", propertyPath};
            }

            const std::string_view parentPath{propertyPath.data(), pos};
            const std::string_view propName{propertyPath.data() + pos + 1, propertyPath.size() - pos - 1};

            return {parentPath, propName};
        }
    }  // namespace

    // this function is used to access to the GlobalProperties instance from test projects only
    // without the need to create application.
    // std::unique_ptr<GlobalProperties> createGlobalProperties()
    // {
    //     return std::make_unique<PropertyContainerImpl>();
    // }

    PropertyContainerImpl::PropertyContainerImpl() :
        m_propsRoot(serialization::jsonCreateDictionary())
    {
        m_propsRoot->as<serialization::JsonValueHolder&>().setGetStringCallback([this](std::string_view str)
        {
            return expandConfigString(str);
        });
    }

    RuntimeValuePtr PropertyContainerImpl::findValueAtPath(std::string_view valuePath) const
    {  // BE AWARE: findValueAtPath requires m_mutex to be locked !
        if (!m_propsRoot)
        {
            return nullptr;
        }

        RuntimeValuePtr current = m_propsRoot;

        for (const std::string_view propName : strings::Split(valuePath, std::string_view{"/"}))
        {
            if (propName.empty())
            {
                continue;
            }
            auto* const currentDict = current->as<ReadonlyDictionary*>();
            if (!currentDict)
            {
                // if (diag::hasLogger())
                // {  // TODO: remove when logger can handle it
                //     NAU_LOG_WARNING("Can not read property ({}) value: the enclosing object is not a dictionary");
                // }
                return nullptr;
            }

            current = currentDict->getValue(propName);
            if (!current)
            {
                return nullptr;
            }
        }

        return current;
    }

    Result<Ptr<Dictionary>> PropertyContainerImpl::getDictionaryAtPath(std::string_view valuePath, bool createPath)
    {
        // BE AWARE: getDictionaryAtPath requires m_mutex lock !
        MY_FATAL(m_propsRoot);

        RuntimeValuePtr current = m_propsRoot;

        for (const std::string_view propName : strings::Split(valuePath, std::string_view{"/"}))
        {
            if (propName.empty())
            {
                continue;
            }
            auto* const currentDict = current->as<Dictionary*>();
            if (!currentDict)
            {
                return MakeError("The enclosing object is not a dictionary", valuePath);
            }

            // std::string_view key = strings::toStringView(propName);

            if (!currentDict->containsKey(propName))
            {
                if (!createPath)
                {
                    return MakeError("Path not exists");
                }

                CheckResult(currentDict->setValue(propName, serialization::jsonCreateDictionary()));
            }

            current = currentDict->getValue(propName);
        }

        if (current && current->is<Dictionary>())
        {
            return current;
        }

        return MakeError("The enclosing object is not a dictionary ({})", valuePath);
    }

    std::optional<std::string> PropertyContainerImpl::expandConfigString(std::string_view str) const
    {
        //  \$([a-zA-Z_0-9\-]*)\{([a-zA-Z_0-9/\-/]*)\}
        //  will parse strings like $[VAR_NAME]{[VAR_VALUE]}
        std::regex re(R"-(\$([a-zA-Z_0-9\-/]*)\{([a-zA-Z_0-9\-/]*)\})-", std::regex_constants::ECMAScript | std::regex_constants::icase);
        std::cmatch match;

        std::string_view currentStr = str;
        std::string result;

        while (std::regex_search(currentStr.data(), currentStr.data() + currentStr.size(), match, re))
        {
            MY_FATAL(match.size() >= 3);

            std::string_view varKind{match[1].first, static_cast<size_t>(match[1].length())};
            std::string_view varValue{match[2].first, static_cast<size_t>(match[2].length())};

            std::string replacementStr;

            if (varKind.empty())
            {
                auto propValue = findValueAtPath(varValue);
                if (propValue && propValue->is<StringValue>())
                {
                    std::string strValue = propValue->as<const StringValue&>().getString();
                    replacementStr.assign(strValue.data(), strValue.size());
                }
            }
            else if (auto resolver = m_variableResolvers.find(varKind); resolver != m_variableResolvers.end())
            {
                std::optional<std::string> resolvedStr = resolver->second(varValue);
                if (resolvedStr)
                {
                    replacementStr = *std::move(resolvedStr);
                }
                else
                {
                    replacementStr.assign(match[0].first, static_cast<size_t>(match[0].length()));
                }
            }
            else
            {
                replacementStr.assign(match[0].first, static_cast<size_t>(match[0].length()));
            }

            if (const auto& prefix = match.prefix(); prefix.length() > 0)
            {
                result.append(prefix.first, static_cast<size_t>(prefix.length()));
            }

            result.append(replacementStr.data(), replacementStr.size());

            const auto& suffix = match.suffix();
            currentStr = std::string_view{suffix.first, static_cast<size_t>(suffix.length())};
        }

        if (result.empty())
        {
            return std::nullopt;
        }

        if (!currentStr.empty())
        {
            result.append(currentStr.data(), currentStr.size());
        }

        return result;
    }

    bool PropertyContainerImpl::contains(std::string_view path) const
    {
        const std::shared_lock lock(m_mutex);
        return findValueAtPath(path) != nullptr;
    }

    RuntimeValuePtr PropertyContainerImpl::getRead(std::string_view path, ReadOnlyLock& lock, [[maybe_unused]] IAllocator* allocator) const
    {
        lock = std::shared_lock{m_mutex};
        return findValueAtPath(path);
    }

    Result<RuntimeValuePtr> PropertyContainerImpl::getModify(std::string_view path, ModificationLock& lock, [[maybe_unused]] IAllocator* allocator)
    {
        MY_FATAL(m_propsRoot);

        lock = std::unique_lock{m_mutex};

        auto [parentPath, propName] = split_property_path(path);

        Result<Ptr<Dictionary>> parentDict = getDictionaryAtPath(parentPath, false);
        CheckResult(parentDict);
        MY_FATAL(*parentDict);

        if (propName.empty())
        {  // the properties root was requested.
            return parentDict;
        }

        if (!(*parentDict)->containsKey(propName))
        {
            return MakeError("To be modifiable the property:() at ({}) must exists first", propName, parentPath);
        }

        RuntimeValuePtr childContainer = (*parentDict)->getValue(propName);

        const bool propertyIsContainer = childContainer->is<Dictionary>() || childContainer->is<Collection>();
        if (!propertyIsContainer)
        {
            return MakeError("Property ({}) expected to be dictionary or collection", propName);
        }

        return childContainer;
    }

    Result<> PropertyContainerImpl::set(std::string_view path, RuntimeValuePtr value)
    {
        MY_FATAL(m_propsRoot);

        const std::lock_guard lock(m_mutex);

        auto [parentPath, propName] = split_property_path(path);

        Result<Ptr<Dictionary>> parentDict = getDictionaryAtPath(parentPath);
        CheckResult(parentDict);
        MY_FATAL(*parentDict);

        return (*parentDict)->setValue(propName, value);
    }

    Result<> PropertyContainerImpl::mergeWithValue(const RuntimeValue& value)
    {
        MY_FATAL(m_propsRoot);

        if (!value.is<ReadonlyDictionary>())
        {
            return MakeError("Dictionary value is expected");
        }

        const std::lock_guard lock(m_mutex);

        // const_cast<> is a temporary hack:. currently RuntimeValue::assign accepts Ptr<> that require only non-const values.
        return RuntimeValue::assign(m_propsRoot, Ptr{&const_cast<RuntimeValue&>(value)}, ValueAssignOption::MergeCollection);
    }

    void PropertyContainerImpl::addVariableResolver(std::string_view kind, VariableResolverCallback resolver)
    {
        MY_DEBUG_ASSERT(!kind.empty());
        MY_DEBUG_ASSERT(resolver);

        if (kind.empty() || !resolver)
        {
            return;
        }

        const std::lock_guard lock(m_mutex);
        [[maybe_unused]] auto [iter, emplaceOk] = m_variableResolvers.emplace(kind, std::move(resolver));
        MY_DEBUG_ASSERT(emplaceOk, "Variable resolver ({}) already exists", kind);
    }

    std::unique_ptr<PropertyContainer> createPropertyContainer()
    {
        return std::make_unique<PropertyContainerImpl>();
    }

    Result<> mergePropertiesFromStream(PropertyContainer& properties, io::IStream& stream, std::string_view contentType)
    {
        if (strings::icaseEqual(contentType, "application/json"))
        {
            Result<RuntimeValuePtr> parseResult = serialization::jsonParse(stream);
            CheckResult(parseResult)

            return properties.mergeWithValue(**parseResult);
        }
        else
        {
            return MakeError("Unknown config's content type:({})", contentType);
        }

        return kResultSuccess;
    }

    Result<> mergePropertiesFromFile(PropertyContainer& properties, const std::filesystem::path& filePath, std::string_view contentType)
    {
        namespace fs = std::filesystem;
        using namespace my::io;

        if (!fs::exists(filePath) && fs::is_regular_file(filePath))
        {
            return MakeError("Path does not exists or not a file:({})", filePath.string());
        }

        if (contentType.empty())
        {
#ifdef _WIN32
            if (strings::icaseEqual(std::wstring_view{filePath.extension().c_str()}, std::wstring_view{L".json"}))
#else
            if (strings::icaseEqual(std::string_view{filePath.extension().c_str()}, std::string_view{".json"}))
#endif
            {
                contentType = "application/json";
            }
            else
            {
                return MakeError("Can not determine file's content type:({})", filePath.string());
            }
        }

        StreamPtr fileStream = createNativeFileStream(filePath, AccessMode::Read, OpenFileMode::OpenExisting);
        if (!fileStream)
        {
            return MakeError("Fail to open file:({})", filePath.string());
        }

        return mergePropertiesFromStream(properties, *fileStream, contentType);
    }

    void dumpPropertiesToStream(PropertyContainer& properties, io::IStream& stream, std::string_view contentType)
    {
        using namespace my::serialization;

        PropertyContainer::ModificationLock lock;

        Result<RuntimeValuePtr> root = properties.getModify("/", lock);
        MY_FATAL(root);

        if (strings::icaseEqual(contentType, "application/json"))
        {
            jsonWrite(stream, *root, JsonSettings{.pretty = true, .writeNulls = true}).ignore();
        }
        else
        {
            MY_FAILURE("Unknown contentType ({})", contentType);
        }
    }

    std::string dumpPropertiesToString(PropertyContainer& properties, std::string_view contentType)
    {
        std::string buffer;
        io::InplaceStringWriter writer{buffer};
        dumpPropertiesToStream(properties, writer, contentType);
        return buffer;
    }
}  // namespace my
