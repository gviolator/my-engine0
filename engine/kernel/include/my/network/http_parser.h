// #my_engine_source_file
#pragma once

#include "my/utils/result.h"
#include "my/kernel/kernel_config.h"
// #include <runtime/diagnostics/mylog.h>
#include <optional>
#include <string_view>

namespace my {

/**
    see
    - https://www.rfc-editor.org/rfc/rfc9110.html  (old: https://www.w3.org/Protocols/rfc2616/rfc2616.html)

*/
class HttpParser
{
public:
    struct Header
    {
        std::string_view key;
        std::string_view value;

        Header();
        Header(std::string_view, std::string_view = std::string_view{});
        explicit operator bool() const;
        bool operator==(const Header&) const;
        //bool operator!=(const Header&) const;
        bool operator==(std::string_view) const;
    };

    constexpr static std::string_view EndOfLine{"\r\n"};

    constexpr static std::string_view EndOfHeaders{"\r\n\r\n"};

    struct HeaderIterator
    {
        using iterator_category = std::input_iterator_tag;
        using value_type = Header;
        using difference_type = ptrdiff_t;
        using pointer = Header*;
        using reference = Header&;

        const HttpParser* parser = nullptr;
        value_type value;

        HeaderIterator();
        HeaderIterator(const HttpParser& parser, const Header&);
        bool operator==(const HeaderIterator&) const;
        bool operator!=(const HeaderIterator&) const;
        HeaderIterator& operator++();
        HeaderIterator operator++(int);
        const Header& operator*() const;
        const Header* operator->() const;
    };

    using iterator = HeaderIterator;

    static std::string_view getHeadersBuffer(std::string_view buffer);

    static Header tryParseHeader(std::string_view line);
    
    static Result<Header> parseHeader(std::string_view line);

    static std::string_view getNextLine(std::string_view buffer, std::string_view line);

    static Header getFirstHeader(std::string_view buffer);

    static Header getNextHeader(std::string_view buffer, const Header& header);

    static Header findHeader(std::string_view buffer, std::string_view key);

    static size_t headersCount(std::string_view buffer);

    static Header headerAt(std::string_view buffer, size_t index);

    // static boost::optional<ResponseStatus> parseStatus(std::string_view buffer);

    // static boost::optional<RequestData> parseRequestData(std::string_view buffer);

    HttpParser();
    HttpParser(const HttpParser&) noexcept = default;
    HttpParser(std::string_view buffer);

    HttpParser& operator=(const HttpParser&) noexcept = default;

    explicit operator bool() const;

    size_t headersLength() const;

    size_t contentLength() const;

    std::string_view operator[](std::string_view) const;

    iterator begin() const;

    iterator end() const;

    bool hasHeader(std::string_view) const;

private:
    std::string_view m_buffer;
    mutable std::optional<size_t> m_contentLength;
};

}  // namespace my

namespace std {

inline auto begin(const my::HttpParser& parser)
{
    return parser.begin();
}

inline auto end(const my::HttpParser& parser)
{
    return parser.end();
}

}  // namespace std

#