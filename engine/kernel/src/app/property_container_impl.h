// #my_engine_source_file

#pragma once

#include "my/app/property_container.h"
#include "my/rtti/rtti_impl.h"
#include "my/utils/string_utils.h"

namespace my
{
    /**
     */
    class PropertyContainerImpl final : public PropertyContainer
    {
        MY_TYPEID(my::PropertyContainerImpl)
        MY_CLASS_BASE(PropertyContainer)

    public:
        PropertyContainerImpl();

        bool contains(std::string_view path) const override;

        RuntimeValuePtr getRead(std::string_view path, ReadOnlyLock& lock,  IMemAllocator* allocator) const override;

        Result<RuntimeValuePtr> getModify(std::string_view path, ModificationLock& lock, IMemAllocator* allocator) override;

        Result<> set(std::string_view path, RuntimeValuePtr value) override;

        Result<> mergeWithValue(const RuntimeValue& value) override;

        void addVariableResolver(std::string_view kind, VariableResolverCallback resolver) override;

    private:
        RuntimeValuePtr findValueAtPath(std::string_view valuePath) const;

        Result<Ptr<RuntimeDictionary>> getDictionaryAtPath(std::string_view valuePath, bool createPath = true);

        std::optional<std::string> expandConfigString(std::string_view str) const;

        Ptr<RuntimeDictionary> m_propsRoot;
        std::map<std::string, VariableResolverCallback, strings::CiStringComparer<std::string_view>> m_variableResolvers;
        mutable std::shared_mutex m_mutex;
    };
}  // namespace my
