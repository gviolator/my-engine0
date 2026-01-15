// #my_engine_source_file
#include "my/utils/string_conv.h"
#include "my/utils/string_utils.h"

namespace my::kernel_detail {

namespace {

template <typename T>
std::basic_string_view<T> SplitNextImpl(std::basic_string_view<T> str, std::basic_string_view<T> current, std::basic_string_view<T> separators)
{
    using StringView = std::basic_string_view<T>;

    if (str.empty())
    {
        return {};
    }

    size_t offset = 0;
    if (current.data() != nullptr)
    {
        MY_DEBUG_ASSERT(str.data() <= current.data() && current.data() < str.data() + str.size());
        offset = static_cast<size_t>(current.data() - str.data());
        offset += (current.size() + 1);
    }

    if (offset == str.size())
    {
        auto result = StringView{str.data() + (str.size() - 1), 0};
        if (result.data() == current.data())
        {
            return StringView{};
        }

        return result;
    }

    // size_t offset = current.empty() ? 0 : (current.data() - str.data()) + current.size();

    if (offset > str.size())
    {
        return {};
    }

    do
    {
        // DEBUG_CHECK(offset < str.size())

        const auto pos = str.find_first_of(separators, offset);

        if (pos == StringView::npos)
        {
            auto result = StringView{str.data() + offset, str.size() - offset};
            return result;
            // break;
        }

        if (pos == offset)
        {  // empty value
            return StringView{str.data() + offset, 0};
        }

        if (pos != offset)
        {
            return StringView(str.data() + offset, pos - offset);
        }

        ++offset;
    }
    while (offset < str.size());

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

std::string_view SplitNext(std::string_view str, std::string_view current, std::string_view separators)
{
    return SplitNextImpl(str, current, separators);
}

std::wstring_view SplitNext(std::wstring_view str, std::wstring_view current, std::wstring_view separators)
{
    return SplitNextImpl(str, current, separators);
}

}  // namespace my::kernel_detail

namespace my::strings {

template <typename T>
std::pair<size_t, std::basic_string_view<T>> StrSplitNext(const std::basic_string_view<T> input, const std::basic_string_view<T> sep, const size_t offset)
{
    constexpr auto npos = std::string_view::npos;
    using SView = std::basic_string_view<T>;

    if (offset == npos || input.empty())
    {
        return {npos, SView{}};
    }

    const auto pos = input.find_first_of(sep, offset);
    if (pos == npos)
    {
        return {
            npos, SView{input.data() + offset, input.size() - offset}
        };
    }

    return {
        pos + 1, SView{input.data() + offset, pos - offset}
    };
};


SplitSequence::iterator::iterator(value_type str, value_type separators, value_type::size_type offset) :
    m_str(str),
    m_separators(separators),
    m_nextOffset(offset)
{
    advance();
}

void SplitSequence::iterator::advance()
{
    std::tie(m_nextOffset, m_value) = StrSplitNext(m_str, m_separators, m_nextOffset);
    if (m_value.data() == nullptr)
    {
        MY_DEBUG_ASSERT(m_nextOffset == value_type::npos);
        m_str = std::string_view{};
        m_separators = std::string_view{};
    }
}

bool SplitSequence::iterator::operator==(const iterator& other) const
{
    if (m_str.empty() || other.m_str.empty())
    {
        return m_str.empty() && other.m_str.empty();
    }

    MY_DEBUG_ASSERT(m_str.data() == other->data());

    return m_nextOffset == other.m_nextOffset;
}

bool SplitSequence::iterator::operator!=(const iterator& other) const
{
    return !this->operator==(other);
}

SplitSequence::iterator& SplitSequence::iterator::operator++()
{
    advance();
    return *this;
}

SplitSequence::iterator SplitSequence::iterator::operator++(int)
{
    auto iter = *this;
    this->operator++();

    return iter;
}

SplitSequence::iterator::value_type SplitSequence::iterator::operator*() const
{
    MY_DEBUG_ASSERT(m_value.data() != nullptr, "Iterator is not dereferenceable");
    return m_value;
}

const SplitSequence::iterator::value_type* SplitSequence::iterator::operator->() const
{
    MY_DEBUG_ASSERT(m_value.data() != nullptr, "Iterator is not dereferenceable");
    return &m_value;
}

SplitSequence::iterator SplitSequence::begin() const
{
    return iterator{str, separators, 0};
}

SplitSequence::iterator SplitSequence::end() const
{
    return iterator{};
}

/**
*/
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
