// #my_engine_source_file

#pragma once
#include "my/io/stream.h"
#include "my/kernel/kernel_config.h"
#include "my/memory/allocator.h"
#include "my/serialization/runtime_value.h"
#include "my/serialization/serialization.h"
#include "my/utils/functor.h"
#include "my/utils/result.h"

#include <json/json.h>

#include <optional>
#include <string>
#include <string_view>

namespace my::serialization {
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
Result<> jsonWrite(io::IStream&, const RuntimeValuePtr&, JsonSettings = {});

/**
 */
MY_KERNEL_EXPORT
Result<RuntimeValuePtr> jsonParse(io::IStream&, IAllocator* = nullptr);

/**
 */
MY_KERNEL_EXPORT
Result<RuntimeValuePtr> jsonParseString(std::string_view, IAllocator* = nullptr);

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
RuntimeValuePtr jsonToRuntimeValue(Json::Value&& root, IAllocator* = nullptr);

/**
 */
inline Ptr<Dictionary> jsonCreateDictionary()
{
    return jsonToRuntimeValue(Json::Value{Json::ValueType::objectValue});
}

/**
 */
inline Ptr<Collection> jsonCreateCollection()
{
    return jsonToRuntimeValue(Json::Value{Json::ValueType::arrayValue});
}

/**
 */
MY_KERNEL_EXPORT
RuntimeValuePtr jsonAsRuntimeValue(const Json::Value& root, IAllocator* = nullptr);

MY_KERNEL_EXPORT
RuntimeValuePtr jsonAsRuntimeValue(Json::Value& root, IAllocator* = nullptr);

MY_KERNEL_EXPORT
Result<> runtimeApplyToJsonValue(Json::Value& jsonValue, const RuntimeValuePtr&, JsonSettings = {});

MY_KERNEL_EXPORT
Json::Value runtimeToJsonValue(const RuntimeValuePtr&, JsonSettings = {});

MY_KERNEL_EXPORT
Result<> jsonWrite(io::IStream&, const Json::Value&, JsonSettings = {});

}  // namespace my::serialization
