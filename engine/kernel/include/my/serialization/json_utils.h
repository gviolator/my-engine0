// #my_engine_source_file

#pragma once
#if !__has_include(<json/json.h>)
    #error json cpp required
#endif

#include "my/io/stream_utils.h"
#include "my/memory/allocator.h"
#include "my/serialization/json.h"
#include "my/serialization/runtime_value_builder.h"
#include "my/utils/string_conv.h"

#include <string>
#include <string_view>
#include <type_traits>

namespace my::serialization {

/*
 */
struct JsonUtils
{
    /**

    */
    template <typename T>
    static inline Result<T> parse(std::reference_wrapper<const Json::Value> jValue)
    {
        // rtstack();
        return runtimeValueCast<T>(jsonAsRuntimeValue(jValue, getDefaultAllocatorPtr()));
    }

    /**

    */
    template <typename T>
    static inline Result<> parse(T& value, std::reference_wrapper<const Json::Value> jValue)
    {
        // rtstack();
        return runtimeValueApply(value, jsonAsRuntimeValue(jValue, getDefaultAllocatorPtr()));
    }

    /**

    */
    template <typename T>
    static inline Result<> parse(T& value, std::string_view jsonString)
    {
        // rtstack();

        Result<RuntimeValuePtr> parseResult = jsonParseString(jsonString, getDefaultAllocatorPtr());
        if (!parseResult)
        {
            return parseResult.getError();
        }

        return runtimeValueApply(value, *parseResult);
    }

    /**
     */
    template <typename T>
    static inline Result<T> parse(std::string_view jsonString)
    {
        // rtstack();

        Result<RuntimeValuePtr> parseResult = jsonParseString(jsonString, getDefaultAllocatorPtr());
        if (!parseResult)
        {
            return parseResult.getError();
        }

        return runtimeValueCast<T>(*parseResult);
    }

    /**
     */
    template <typename Char = char>
    static inline std::basic_string<Char> stringify(const auto& value, JsonSettings settings = {})
    {
        // rtstack();
        std::basic_string<Char> buffer;
        io::InplaceStringWriter writer{buffer};
        serialization::jsonWrite(writer, makeValueRef(value, getDefaultAllocatorPtr()), settings).ignore();

        return buffer;
    }

    /**
     */
    template <typename T>
    static inline Json::Value toJsonValue(const T& value)
    {
        // rtstack();
        return runtimeToJsonValue(makeValueRef(value, getDefaultAllocatorPtr()));
    }
};

}  // namespace my::serialization
