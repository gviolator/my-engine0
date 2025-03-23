// #my_engine_source_header

#pragma once
#include <EASTL/optional.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>

#include "my/io/stream.h"
#include "my/kernel/kernel_config.h"
#include "my/memory/mem_allocator.h"
#include "my/serialization/runtime_value.h"
#include "my/serialization/serialization.h"
#include "my/utils/functor.h"
#include "my/utils/result.h"


#if __has_include(<json/json.h>)
    #define HAS_JSONCPP
    #include <json/json.h>
#endif

namespace my::serialization
{
    /**
     */
    struct JsonSettings
    {
        bool pretty = false;
        bool writeNulls = false;
    };

    /**
     */
    MY_KERNEL_EXPORT
    Result<> jsonWrite(io::IStreamWriter&, const RuntimeValue::Ptr&, JsonSettings = {});

    /**
     */
    MY_KERNEL_EXPORT
    Result<RuntimeValue::Ptr> jsonParse(io::IStreamReader&, IMemAllocator::Ptr = nullptr);

    /**
     */
    MY_KERNEL_EXPORT
    Result<RuntimeValue::Ptr> jsonParseString(std::string_view, IMemAllocator::Ptr = nullptr);

    /**
     */
    inline Result<RuntimeValue::Ptr> jsonParseString(std::u8string_view str, IMemAllocator::Ptr allocator = nullptr)
    {
        return jsonParseString(std::string_view{reinterpret_cast<const char*>(str.data()), str.size()}, std::move(allocator));
    }

#ifdef HAS_JSONCPP

    /**
     */
    struct MY_ABSTRACT_TYPE JsonValueHolder
    {
        using GetStringCallback = Functor<std::optional<std::string>(std::string_view)>;

        MY_TYPEID(my::serialization::JsonValueHolder)

        virtual Json::Value& getRootJsonValue() = 0;
        virtual const Json::Value& getRootJsonValue() const = 0;
        virtual Json::Value& getThisJsonValue() = 0;
        virtual const Json::Value& getThisJsonValue() const = 0;
        virtual void setGetStringCallback(GetStringCallback callback) = 0;
    };

    /**
     */
    MY_KERNEL_EXPORT
    Json::CharReader& jsonGetParser();

    MY_KERNEL_EXPORT
    Result<Json::Value> jsonParseToValue(std::string_view jsonString);

    inline Result<Json::Value> jsonParseToValue(std::u8string_view jsonString)
    {
        return jsonParseToValue(std::string_view{reinterpret_cast<const char*>(jsonString.data()), jsonString.size()});
    }

    /**
     */
    MY_KERNEL_EXPORT
    RuntimeValue::Ptr jsonToRuntimeValue(Json::Value&& root, IMemAllocator::Ptr = nullptr);

    /**
     */
    inline RuntimeDictionary::Ptr jsonCreateDictionary()
    {
        return jsonToRuntimeValue(Json::Value{Json::ValueType::objectValue});
    }

    /**
     */
    inline RuntimeCollection::Ptr jsonCreateCollection()
    {
        return jsonToRuntimeValue(Json::Value{Json::ValueType::arrayValue});
    }

    /**
     */
    MY_KERNEL_EXPORT
    RuntimeValue::Ptr jsonAsRuntimeValue(const Json::Value& root, IMemAllocator::Ptr = nullptr);

    MY_KERNEL_EXPORT
    RuntimeValue::Ptr jsonAsRuntimeValue(Json::Value& root, IMemAllocator::Ptr = nullptr);

    MY_KERNEL_EXPORT
    Result<> runtimeApplyToJsonValue(Json::Value& jsonValue, const RuntimeValue::Ptr&, JsonSettings = {});

    MY_KERNEL_EXPORT
    Json::Value runtimeToJsonValue(const RuntimeValue::Ptr&, JsonSettings = {});

    MY_KERNEL_EXPORT
    Result<> jsonWrite(io::IStreamWriter&, const Json::Value&, JsonSettings = {});

#endif

}  // namespace my::serialization

#ifdef HAS_JSONCPP
    #undef HAS_JSONCPP
#endif