// #my_engine_source_header
// nau/serialization/json_utils.h


#pragma once
#if !__has_include(<json/json.h>)
    #error json cpp required
#endif

#include <EASTL/string.h>
#include <EASTL/string_view.h>

#include <string_view>
#include <type_traits>

#include "my/io/stream_utils.h"
#include "my/string/string_conv.h"
#include "my/memory/mem_allocator.h"
#include "my/serialization/json.h"
#include "my/serialization/runtime_value_builder.h"

namespace my::serialization
{

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
            return runtimeValueCast<T>(jsonAsRuntimeValue(jValue, getDefaultAllocator()));
        }

        /**

        */
        template <typename T>
        static inline Result<> parse(T& value, std::reference_wrapper<const Json::Value> jValue)
        {
            // rtstack();
            return runtimeValueApply(value, jsonAsRuntimeValue(jValue, getDefaultAllocator()));
        }

        /**

        */
        template <typename T>
        static inline Result<> parse(T& value, std::u8string_view jsonString)
        {
            // rtstack();

            Result<RuntimeValue::Ptr> parseResult = jsonParseString(jsonString, getDefaultAllocator());
            if(!parseResult)
            {
                return parseResult.getError();
            }

            return runtimeValueApply(value, *parseResult);
        }

        /**
         */
        template <typename T>
        static inline Result<T> parse(std::u8string_view jsonString)
        {
            // rtstack();

            Result<RuntimeValue::Ptr> parseResult = jsonParseString(jsonString, getDefaultAllocator());
            if(!parseResult)
            {
                return parseResult.getError();
            }

            return runtimeValueCast<T>(*parseResult);
        }

        template <typename T>
        static inline Result<> parse(T& value, std::string_view jsonString)
        {
            if(jsonString.empty())
            {
                return MakeError("Empty string");
            }

            const char8_t* const utfPtr = reinterpret_cast<const char8_t*>(jsonString.data());
            return parse(value, std::u8string_view{utfPtr, jsonString.size()});
        }

        template <typename T>
        static inline Result<T> parse(std::string_view jsonString)
        {
            if(jsonString.empty())
            {
                return MakeError("Empty string");
            }

            const char8_t* const utfPtr = reinterpret_cast<const char8_t*>(jsonString.data());
            return parse<T>(std::u8string_view{utfPtr, jsonString.size()});
        }

        /**
         */
        template <typename Char = char8_t>
        static inline std::basic_string<Char> stringify(const auto& value, JsonSettings settings = {})
        {
            // rtstack();
            std::basic_string<Char> buffer;
            io::InplaceStringWriter<Char> writer{buffer};
            serialization::jsonWrite(writer, makeValueRef(value, getDefaultAllocator()), settings).ignore();

            return buffer;
        }

        /**
         */
        template <typename T>
        static inline Json::Value toJsonValue(const T& value)
        {
            // rtstack();
            return runtimeToJsonValue(makeValueRef(value, getDefaultAllocator()));
        }
    };

}  // namespace my::serialization
