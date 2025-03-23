// #my_engine_source_header
#pragma once
#include <concepts>
#include <string>

#include "my/utils/result.h"

namespace my
{
    template <typename T>
    concept StringConvertible = requires(const T& value) {
        { toString(value) } -> std::convertible_to<std::string>;
    };

    template <typename T>
    concept StringParsable = requires(T& value) {
        { parse(std::string_view{}, value) } -> std::same_as<Result<>>;
    };

    template <typename T>
    concept WStringConvertible = requires(const T& value) {
        { toWString(value) } -> std::convertible_to<std::wstring>;
    };

    template <typename T>
    concept WStringParsable = requires(T& value) {
        { parse(std::wstring_view{}, value) } -> std::same_as<Result<>>;
    };
}  // namespace my
