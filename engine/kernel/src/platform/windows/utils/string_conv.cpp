// #my_engine_source_file
#include "my/utils/string_conv.h"

#include "my/diag/assert.h"

namespace my::strings
{
    std::wstring utf8ToWString(std::string_view text)
    {
        if (text.empty())
        {
            return {};
        }

        const char* const inTextPtr = reinterpret_cast<const char*>(text.data());
        const int inTextLen = static_cast<int>(text.size());

        int len = ::MultiByteToWideChar(CP_UTF8, 0, inTextPtr, inTextLen, nullptr, 0);
        MY_DEBUG_FATAL(len > 0, "Invalid input UTF-8 string ({})", text);
        if (len == 0)
        {
            return {};
        }

        std::wstring result;
        result.resize(static_cast<size_t>(len));

        len = ::MultiByteToWideChar(CP_UTF8, 0, inTextPtr, inTextLen, result.data(), static_cast<int>(result.size()));
        return len > 0 ? result : std::wstring{};
    }

    std::string wstringToUtf8(std::wstring_view text)
    {
        if (text.empty())
        {
            return {};
        }

        const wchar_t* const inTextPtr = text.data();
        const int inTextLen = static_cast<int>(text.size());

        const char* defaultChr = "?";
        BOOL defaultChrUsed = FALSE;

        int len = ::WideCharToMultiByte(CP_UTF8, 0, inTextPtr, inTextLen, nullptr, 0, defaultChr, &defaultChrUsed);
        MY_DEBUG_FATAL(len > 0, "Invalid input wstring");
        if (len == 0)
        {
            return {};
        }

        std::string result;
        result.resize(static_cast<size_t>(len));

        len = ::WideCharToMultiByte(CP_UTF8, 0, inTextPtr, inTextLen, result.data(), static_cast<int>(result.size()), defaultChr, &defaultChrUsed);
        return len > 0 ? result : std::string{""};
    }
}  // namespace my::strings