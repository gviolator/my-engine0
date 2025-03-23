// #my_engine_source_header
#include "my/platform/windows/diag/win_error.h"

#include "my/utils/scope_guard.h"
#include "my/utils/string_conv.h"

namespace my::diag
{
  namespace
  {
    // https://msdn.microsoft.com/en-us/library/ms679351(v=VS.85).aspx
    inline DWORD formatMessageHelper(DWORD messageId, LPWSTR& buffer)
    {
      return FormatMessageW(
          FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
          nullptr, messageId,
          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
          reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
    }

    inline DWORD formatMessageHelper(DWORD messageId, LPSTR& buffer)
    {
      return FormatMessageA(
          FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
          nullptr, messageId,
          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
          reinterpret_cast<LPSTR>(&buffer), 0, nullptr);
    }

    template <typename Char>
    std::basic_string<Char> getWindowsMessage(DWORD messageId)
    {
      if (messageId == 0)
      {
        return {};
      }

      Char* messageBuffer = nullptr;
      scope_on_leave
      {
        if (messageBuffer)
        {
          ::LocalFree(messageBuffer);
        }
      };

      std::basic_string<Char> resultMessage;
      const DWORD length = formatMessageHelper(messageId, messageBuffer);
      if (length == 0)
      {
        SetLastError(0);
      }
      else if (messageBuffer != nullptr)
      {
        resultMessage.assign(messageBuffer, static_cast<size_t>(length));
      }

      return resultMessage;
    }

    inline std::string makeWinErrorMessage(unsigned errorCode)
    {
      const std::wstring wcsMessage = getWindowsMessage<wchar_t>(errorCode);
      return strings::wstringToUtf8(wcsMessage);
    }

    inline std::string makeWinErrorMessage(unsigned errorCode, std::string_view customMessage)
    {
      const std::wstring wcsMessage = getWindowsMessage<wchar_t>(errorCode);
      const std::string errorMessage = strings::wstringToUtf8(wcsMessage);
      return std::format("{}. code:({}):{}", customMessage, errorCode, errorMessage);
    }
  }  // namespace

  WinCodeError::WinCodeError(const diag::SourceInfo& sourceInfo, unsigned errorCode) :
      DefaultError<>(sourceInfo, makeWinErrorMessage(errorCode)),
      m_errorCode(errorCode)
  {
  }

  WinCodeError::WinCodeError(const diag::SourceInfo& sourceInfo, std::string message, unsigned errorCode) :
      DefaultError<>(sourceInfo, makeWinErrorMessage(errorCode, message)),
      m_errorCode(errorCode)
  {
  }

  unsigned WinCodeError::getErrorCode() const
  {
    return m_errorCode;
  }

  unsigned getAndResetLastErrorCode()
  {
    const auto error = GetLastError();
    if (error != 0)
    {
      SetLastError(0);
    }

    return error;
  }

  std::wstring getWinErrorMessageW(unsigned errorCode)
  {
    return getWindowsMessage<wchar_t>(errorCode);
  }

  std::string getWinErrorMessageA(unsigned errorCode)
  {
    return getWindowsMessage<char>(errorCode);
  }
}  // namespace my::diag
