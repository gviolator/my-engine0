// #my_engine_source_header
#include "my/utils/string_utils.h"

#include "my/utils/string_conv.h"

namespace my::kernel_detail
{

    namespace
    {
        template <typename T>
        std::basic_string_view<T> splitNextImpl(std::basic_string_view<T> str, std::basic_string_view<T> current, std::basic_string_view<T> separators)
        {
            using StringView = std::basic_string_view<T>;

            if (str.empty())
            {
                return {};
            }

            size_t offset = current.empty() ? 0 : (current.data() - str.data()) + current.size();

            if (offset == str.size())
            {
                return {};
            }

            do
            {
                // DEBUG_CHECK(offset < str.size())

                const auto pos = str.find_first_of(separators, offset);

                if (pos == StringView::npos)
                {
                    break;
                }

                if (pos != offset)
                {
                    return StringView(str.data() + offset, pos - offset);
                }

                ++offset;
            } while (offset < str.size());

            return offset == str.size() ? StringView{} : StringView(str.data() + offset, str.size() - offset);
        }

        template <typename T>
        std::basic_string_view<T> trimEndImpl(std::basic_string_view<T> str)
        {
            constexpr T Space = TYPED_STR(T, ' ');

            size_t spaces = 0;

            for (auto r = str.rbegin(); r != str.rend(); ++r)
            {
                if (*r != Space)
                {
                    break;
                }
                ++spaces;
            }

            return spaces == 0 ? str : std::basic_string_view<T>{str.data(), str.length() - spaces};
        }

        template <typename T>
        std::basic_string_view<T> trimStartImpl(std::basic_string_view<T> str)
        {
            constexpr T Space = TYPED_STR(T, ' ');

            auto iter = str.begin();

            for (; iter != str.end(); ++iter)
            {
                if (*iter != Space)
                {
                    break;
                }
            }

            const auto offset = static_cast<size_t>(std::distance(str.begin(), iter));

            if (offset == str.length())
            {
                return {};
            }

            return offset == 0 ? str : std::basic_string_view<T>{str.data() + offset, (str.length() - offset)};
        }

        template <typename T>
        std::basic_string_view<T> trimIMpl(std::basic_string_view<T> str)
        {
            return trimStartImpl(trimEndImpl(str));
        }

        // template <typename T>
        // bool endWithImpl(std::basic_string_view<T> str, std::basic_string_view<T> value)
        // {
        //     const auto pos = str.rfind(value);
        //     return (pos != std::basic_string_view<T>::npos) && (str.length() - pos) == value.length();
        // }

        // template <typename T>
        // bool startWithImpl(std::basic_string_view<T> str, std::basic_string_view<T> value)
        // {
        //     return str.find(value) == 0;
        // }

    }  // namespace

    std::string_view splitNext(std::string_view str, std::string_view current, std::string_view separators)
    {
        return splitNextImpl(str, current, separators);
    }

    std::wstring_view splitNext(std::wstring_view str, std::wstring_view current, std::wstring_view separators)
    {
        return splitNextImpl(str, current, separators);
    }

}  // namespace my::kernel_detail

namespace my::strings
{
    std::pair<std::string_view, std::string_view> cut(std::string_view str, char separator)
    {
        if (str.size() < 2)
        {
            return {str, std::string_view{}};
        }

        const auto index = str.find(separator);
        if (index == std::string_view::npos)
        {
            return {str, std::string_view{}};
        }

        auto left = str.substr(0, index);
        auto right = str.substr(index + 1, str.size() - index - 1);

        return {left, right};
    }

    std::string_view trimEnd(std::string_view str)
    {
        return kernel_detail::trimEndImpl(str);
    }

    std::wstring_view trimEnd(std::wstring_view str)
    {
        return kernel_detail::trimEndImpl(str);
    }

    std::string_view trimStart(std::string_view str)
    {
        return kernel_detail::trimStartImpl(str);
    }

    std::wstring_view trimStart(std::wstring_view str)
    {
        return kernel_detail::trimStartImpl(str);
    }

    std::string_view trim(std::string_view str)
    {
        return kernel_detail::trimIMpl(str);
    }

    std::wstring_view trim(std::wstring_view str)
    {
        return kernel_detail::trimIMpl(str);
    }
}  // namespace my::strings
