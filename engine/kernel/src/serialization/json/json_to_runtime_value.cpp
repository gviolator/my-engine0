// #my_engine_source_file

#include "json_to_runtime_value.h"

#include "my/io/stream.h"
#include "my/memory/buffer.h"
#include "my/serialization/json.h"
#include "my/serialization/runtime_value_builder.h"

namespace my::json_detail
{

    RuntimeValuePtr wrapJsonValueAsCollection(const Ptr<JsonValueHolderImpl>& root, Json::Value& jsonValue);

    RuntimeValuePtr wrapJsonValueAsDictionary(const Ptr<JsonValueHolderImpl>& root, Json::Value& jsonValue);

    RuntimeValuePtr createJsonNullValue(const Ptr<JsonValueHolderImpl>& root);

    Result<> setJsonValue(Json::Value& jsonValue, RuntimeValue& rtValue)
    {
        if (auto* const optValue = rtValue.as<RuntimeOptionalValue*>())
        {
            if (optValue->hasValue())
            {
                CheckResult(setJsonValue(jsonValue, *optValue->getValue()));
            }
            else
            {
                jsonValue = Json::Value::nullSingleton();
            }
        }
        else if (const auto* const intValue = rtValue.as<const RuntimeIntegerValue*>())
        {
            const bool isLong = intValue->getBitsCount() == sizeof(int64_t);

            if (intValue->isSigned())
            {
                const int64_t i = intValue->getInt64();
                jsonValue = isLong ? Json::Value{static_cast<Json::Int64>(i)} : Json::Value{static_cast<Json::Int>(i)};
            }
            else
            {
                const uint64_t i = intValue->getUint64();
                jsonValue = isLong ? Json::Value{static_cast<Json::UInt64>(i)} : Json::Value{static_cast<Json::UInt>(i)};
            }
        }
        else if (const auto* const floatValue = rtValue.as<const RuntimeFloatValue*>())
        {
            jsonValue = Json::Value{floatValue->getDouble()};
        }
        else if (const auto* const boolValue = rtValue.as<const RuntimeBooleanValue*>())
        {
            jsonValue = Json::Value{boolValue->getBool()};
        }
        else if (const auto* const strValue = rtValue.as<RuntimeStringValue*>())
        {
            jsonValue = Json::Value{strValue->getString()};
        }
        else if (auto* coll = rtValue.as<RuntimeReadonlyCollection*>(); coll)
        {
            if (jsonValue.type() != Json::ValueType::arrayValue)
            {
                jsonValue = Json::Value{Json::ValueType::arrayValue};
            }

            for (size_t i = 0, size = coll->getSize(); i < size; ++i)
            {
                Json::Value elementJsonValue;

                if (RuntimeValuePtr elementValue = coll->getAt(i))
                {
                    CheckResult(setJsonValue(elementJsonValue, *elementValue));
                }

                jsonValue.append(std::move(elementJsonValue));
            }
        }
        else if (auto* dict = rtValue.as<RuntimeReadonlyDictionary*>(); dict)
        {
            if (jsonValue.type() != Json::ValueType::objectValue)
            {
                jsonValue = Json::Value{Json::ValueType::objectValue};
            }

            for (size_t i = 0, size = dict->getSize(); i < size; ++i)
            {
                const auto [key, fieldValue] = (*dict)[i];

                Json::Value* const field = jsonValue.demand(key.data(), key.data() + key.size());
                MY_FATAL(field);

                if (fieldValue)
                {
                    CheckResult(setJsonValue(*field, *fieldValue));
                }
                else
                {
                    *field = Json::Value::nullSingleton();
                }
            }
        }

        return ResultSuccess;
    }

    RuntimeValuePtr getValueFromJson(const my::Ptr<JsonValueHolderImpl>& root, Json::Value& jsonValue)
    {
        if (jsonValue.isNull())
        {
            return createJsonNullValue(root);
        }
        else if (jsonValue.isUInt())
        {
            return makeValueCopy(jsonValue.asUInt());
        }
        else if (jsonValue.isInt())
        {
            return makeValueCopy(jsonValue.asInt());
        }
        else if (jsonValue.isUInt64())
        {
            return makeValueCopy(jsonValue.asUInt64());
        }
        else if (jsonValue.isInt64())
        {
            return makeValueCopy(jsonValue.asInt64());
        }
        else if (jsonValue.isDouble())
        {
            return makeValueCopy(jsonValue.asDouble());
        }
        else if (jsonValue.isBool())
        {
            return makeValueCopy(jsonValue.asBool());
        }
        else if (jsonValue.isString())
        {
            std::string str = jsonValue.asString();

            if (root)
            {
                if (std::optional<std::string> newString = root->transformString(str); newString)
                {
                    return makeValueCopy(std::move(*newString));
                }
            }

            return makeValueCopy(std::move(str));
        }
        else if (jsonValue.isArray())
        {
            return wrapJsonValueAsCollection(root, jsonValue);
        }
        else if (jsonValue.isObject())
        {
            return wrapJsonValueAsDictionary(root, jsonValue);
        }

        MY_FAILURE("Don't known how to encode jsonValue. Type: ({})", static_cast<int>(jsonValue.type()));
        return nullptr;
    }

    /**
     */
    class JsonNull final : public JsonValueHolderImpl,
                           public RuntimeOptionalValue
    {
        MY_REFCOUNTED_CLASS(my::json_detail::JsonNull, JsonValueHolderImpl, RuntimeOptionalValue)

    public:
        JsonNull(const my::Ptr<JsonValueHolderImpl>& root) :
            JsonValueHolderImpl(root)
        {
        }

        bool isMutable() const override
        {
            return false;
        }

        bool hasValue() const override
        {
            return false;
        }

        RuntimeValuePtr getValue() override
        {
            return nullptr;
        }

        Result<> setValue([[maybe_unused]] RuntimeValuePtr value) override
        {
            return MakeError("Attempt to modify non mutable json value");
        }
    };

    /**
     */
    class JsonCollection final : public JsonValueHolderImpl,
                                 public RuntimeCollection
    {
        MY_REFCOUNTED_CLASS(my::json_detail::JsonCollection, JsonValueHolderImpl, RuntimeCollection)

    public:
        JsonCollection() = default;

        JsonCollection(Json::Value&& value) :
            JsonValueHolderImpl(std::move(value))
        {
        }

        JsonCollection(Json::Value& value) :
            JsonValueHolderImpl(value)
        {
        }

        JsonCollection(const my::Ptr<JsonValueHolderImpl>& root, Json::Value& value) :
            JsonValueHolderImpl(root, value)
        {
        }

        bool isMutable() const override
        {
            return m_isMutable;
        }

        size_t getSize() const override
        {
            const auto& jsonValue = getThisJsonValue();
            MY_DEBUG_CHECK(jsonValue.type() == Json::ValueType::arrayValue);
            return static_cast<size_t>(jsonValue.size());
        }

        RuntimeValuePtr getAt(size_t index) override
        {
            MY_DEBUG_CHECK(getThisJsonValue().type() == Json::ValueType::arrayValue);
            MY_DEBUG_CHECK(index < getSize(), "Invalid index [{}]", index);
            if (index >= getSize())
            {
                return nullptr;
            }

            const auto arrIndex = static_cast<Json::ArrayIndex>(index);
            return getValueFromJson(getRoot(), getThisJsonValue()[arrIndex]);
        }

        Result<> setAt(size_t index, const RuntimeValuePtr& value) override
        {
            MY_DEBUG_CHECK(value);
            if (!value)
            {
                return MakeError("Value is null");
            }

            MY_DEBUG_CHECK(index < getSize());
            if (index >= getSize())
            {
                return MakeError("Invalid index ({})", index);
            }

            const auto arrIndex = static_cast<Json::ArrayIndex>(index);
            return setJsonValue(getThisJsonValue()[arrIndex], *value);
        }

        void clear() override
        {
            getThisJsonValue().clear();
        }

        void reserve([[maybe_unused]] size_t capacity) override
        {
        }

        Result<> append(const RuntimeValuePtr& value) override
        {
            MY_DEBUG_CHECK(getThisJsonValue().type() == Json::ValueType::arrayValue);
            MY_DEBUG_CHECK(value);
            if (!value)
            {
                return MakeError("Value is null");
            }

            Json::Value newValue;
            CheckResult(setJsonValue(newValue, *value));
            getThisJsonValue().append(std::move(newValue));

            return ResultSuccess;
        }
    };

    /**
     */
    class JsonDictionary : public JsonValueHolderImpl,
                           public RuntimeDictionary
    {
        MY_REFCOUNTED_CLASS(my::json_detail::JsonDictionary, JsonValueHolderImpl, RuntimeDictionary)
    public:
        JsonDictionary() = default;

        JsonDictionary(Json::Value&& value) :
            JsonValueHolderImpl(std::move(value))
        {
        }

        JsonDictionary(Json::Value& value) :
            JsonValueHolderImpl(value)
        {
        }

        JsonDictionary(const my::Ptr<JsonValueHolderImpl>& root, Json::Value& value) :
            JsonValueHolderImpl(root, value)
        {
        }

        bool isMutable() const override
        {
            return m_isMutable;
        }

        size_t getSize() const override
        {
            const auto& jsonValue = getThisJsonValue();
            MY_DEBUG_CHECK(jsonValue.type() == Json::ValueType::objectValue);
            return static_cast<size_t>(jsonValue.size());
        }

        std::string_view getKey(size_t index) const override
        {
            const auto& jsonValue = getThisJsonValue();
            MY_DEBUG_CHECK(jsonValue.type() == Json::ValueType::objectValue);
            MY_DEBUG_CHECK(index < jsonValue.size(), "Invalid index ({}) > size:({})", index, jsonValue.size());

            auto iter = jsonValue.begin();
            std::advance(iter, index);
            const char* end;
            const char* const start = iter.memberName(&end);
            return std::string_view{start, static_cast<size_t>(end - start)};
        }

        RuntimeValuePtr getValue(std::string_view key) override
        {
            if (key.empty())
            {
                return nullptr;
            }

            auto& jsonValue = getThisJsonValue();
            MY_DEBUG_CHECK(jsonValue.type() == Json::ValueType::objectValue);

            if (jsonValue.find(key.data(), key.data() + key.size()) == nullptr)
            {
                return nullptr;
            }

            // Value::demand will insert empty default value, so it used only after find.
            Json::Value* const field = jsonValue.demand(key.data(), key.data() + key.size());
            MY_FATAL(field);

            return getValueFromJson(getRoot(), *field);
        }

        Result<> setValue(std::string_view key, const RuntimeValuePtr& value) override
        {
            MY_DEBUG_CHECK(value);
            if (!value)
            {
                return MakeError("Value is null");
            }

            MY_DEBUG_CHECK(!key.empty());
            if (key.empty())
            {
                return MakeError("key is empty");
            }

            Json::Value* const field = getThisJsonValue().demand(key.data(), key.data() + key.size());
            MY_FATAL(field);

            return setJsonValue(*field, *value);
        }

        bool containsKey(std::string_view key) const override
        {
            if (key.empty())
            {
                return false;
            }

            MY_DEBUG_CHECK(getThisJsonValue().type() == Json::ValueType::objectValue);

            return getThisJsonValue().find(key.data(), key.data() + key.size()) != nullptr;
        }

        void clear() override
        {
            MY_DEBUG_CHECK(getThisJsonValue().type() == Json::ValueType::objectValue);
            getThisJsonValue().clear();
        }

        RuntimeValuePtr erase(std::string_view key) override
        {
            MY_FAILURE("JsonDictionary::erase({}) is not implemented", key);
            return nullptr;
        }
    };

    RuntimeValuePtr wrapJsonValueAsCollection(const Ptr<JsonValueHolderImpl>& root, Json::Value& jsonValue)
    {
        return rtti::createInstance<JsonCollection>(root, jsonValue);
    }

    RuntimeValuePtr wrapJsonValueAsDictionary(const Ptr<JsonValueHolderImpl>& root, Json::Value& jsonValue)
    {
        return rtti::createInstance<JsonDictionary>(root, jsonValue);
    }

    RuntimeValuePtr createJsonNullValue(const Ptr<JsonValueHolderImpl>& root)
    {
        return rtti::createInstance<JsonNull>(root);
    }

    Ptr<RuntimeDictionary> createJsonDictionary(Json::Value&& jsonValue)
    {
        return rtti::createInstance<JsonDictionary>(std::move(jsonValue));
    }

    Ptr<RuntimeCollection> createJsonCollection(Json::Value&& jsonValue)
    {
        return rtti::createInstance<JsonCollection>(std::move(jsonValue));
    }

    Ptr<RuntimeDictionary> wrapJsonDictionary(Json::Value& jsonValue)
    {
        return rtti::createInstance<JsonDictionary>(jsonValue);
    }

    Ptr<RuntimeCollection> wrapJsonCollection(Json::Value& jsonValue)
    {
        return rtti::createInstance<JsonCollection>(jsonValue);
    }

}  // namespace my::json_detail

namespace my::serialization
{
    RuntimeValuePtr jsonToRuntimeValue(Json::Value&& root, IMemAllocator*)
    {
        if (root.isObject())
        {
            return json_detail::createJsonDictionary(std::move(root));
        }
        else if (root.isArray())
        {
            return json_detail::createJsonCollection(std::move(root));
        }

        return json_detail::getValueFromJson(nullptr, root);
    }

    RuntimeValuePtr jsonAsRuntimeValue(Json::Value& root, IMemAllocator*)
    {
        if (root.isObject())
        {
            return json_detail::wrapJsonDictionary(root);
        }
        else if (root.isArray())
        {
            return json_detail::wrapJsonCollection(root);
        }

        return nullptr;
    }

    RuntimeValuePtr jsonAsRuntimeValue(const Json::Value& root, IMemAllocator*)
    {
        auto value = jsonAsRuntimeValue(const_cast<Json::Value&>(root));
        value->as<json_detail::JsonValueHolderImpl&>().setMutable(false);

        return value;
    }
}  // namespace my::serialization
