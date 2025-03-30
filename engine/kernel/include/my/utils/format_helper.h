// #my_engine_source_file
#pragma once
#include <concepts>
#include <format>
#include <string>
#include <string_view>

#include "my/utils/string_conv.h"
#include "my/utils/to_string.h"

namespace std
{
    template <typename T>
    requires(my::StringConvertible<T>)
    struct formatter<T, char>
    {
        constexpr auto parse(format_parse_context& context)
        {
            return context.begin();
        }

        auto format(const my::StringConvertible auto& value, format_context& context) const
        {
            std::string str = toString(value);
            return vformat_to(context.out(), "{}", make_format_args(str));
        }
    };

    template <typename T>
    requires(std::constructible_from<std::wstring_view, T>)
    struct formatter<T, char>
    {
        constexpr auto parse(format_parse_context& context)
        {
            return context.begin();
        }

        auto format(const auto& value, format_context& context) const
        {
            const std::wstring_view wstr(value);
            const std::string str = my::strings::wstringToUtf8(wstr);
            return vformat_to(context.out(), "{}", make_format_args(str));
        }
    };
}  // namespace std
