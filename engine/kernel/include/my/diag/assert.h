// #my_engine_source_file

#pragma once

#include "my/debug/debugger.h"
#include "my/diag/source_info.h"
#include "my/kernel/kernel_config.h"
#include "my/utils/preprocessor.h"
#include "my/utils/string_conv.h"
#include "my/utils/typed_flag.h"

#include <format>
#include <type_traits>


namespace my::diag {
enum class AssertionKind
{
    Default,
    Fatal
};

enum class FailureAction
{
    None = FlagValue(0),
    DebugBreak = FlagValue(1),
    Abort = FlagValue(2)
};

MY_DEFINE_TYPED_FLAG(FailureAction)

}  // namespace my::diag

namespace my::diag_detail {
consteval inline std::string_view makeFailureMessage()
{
    return std::string_view{};
}

/*  template <typename T>
  requires(!std::is_constructible_v<std::string_view, T>)
  inline auto makeFormatableArgs(T&& arg)
  {
      return std::forward<T>(arg);
  }

  template <typename T>
  requires std::is_constructible_v<std::string_view, T>
  inline const char* makeFormatableArgs(T&& arg)
  {
      const char* res = std::string_view{std::forward<T>(arg)}.data();
      if (!res)
      {
          return "NULLPTR";
      }
      return res;
  }

  template <typename T>
  requires(!std::is_constructible_v<std::string_view, T>) &&
          std::is_constructible_v<std::string_view, T>
  inline const char* makeFormatableArgs(T&& arg)
  {
      const char* res = std::string_view{std::forward<T>(arg)}.data();
      if (!res)
      {
          return "NULLPTR";
      }
      return res;
  }*/

// template <typename T>
inline const char* makeFormatableArgs(nullptr_t)
{
    return "NULLPTR";
}

inline std::string_view makeFailureMessage(std::string_view str)
{
    return str;
}

template <typename... Args>
inline std::string_view makeFailureMessage(std::string_view message, const Args&... args)
{
    if constexpr (sizeof...(Args) == 0)
    {
        return {};
    }
    else
    {
        constexpr size_t FailureMessageLenMax = 512;
        static thread_local char buffer[FailureMessageLenMax];
        // std::format_to_n_result result = std::format_to_n(buffer, sizeof(buffer), message, args ...);

        auto end = std::vformat_to(buffer, message, std::make_format_args(args...));
        *end = 0;
        return std::string_view{buffer, end};
        // return std::string_view {buffer, result.out};
        // std::string formattedMessage = std::vformat_n_t(std::string_view{message}, std::make_format_args(formatArgs...));
        // return formattedMessage;
        // return std::format(message, makeFormatableArgs<Args>(formatArgs)...);
    }
}

template <typename... Args>
inline std::string makeFailureMessage(const wchar_t* message, const Args&... formatArgs)
{
    if constexpr (sizeof...(Args) == 0)
    {
        return strings::wstringToUtf8(message);
    }
    else
    {
        const std::wstring formattedMessage = std::vformat(std::wstring_view{message}, std::make_format_args(formatArgs...));
        // return makeFailureMessage(strings::wstringToUtf8(formattedMessage));
        return "FIXME";
    }
}

/**
 */
MY_KERNEL_EXPORT
diag::FailureActionFlag raiseFailure(diag::AssertionKind kind, diag::SourceInfo source, std::string_view condition, std::string_view message);

}  // namespace my::diag_detail

#define MY_ASSERT_IMPL(kind, condition, ...)                                                                                                                       \
    do                                                                                                                                                             \
    {                                                                                                                                                              \
        if (!(condition)) [[unlikely]]                                                                                                                             \
        {                                                                                                                                                          \
            const auto errorFlags = ::my::diag_detail::raiseFailure(kind, MY_INLINED_SOURCE_INFO, #condition, ::my::diag_detail::makeFailureMessage(__VA_ARGS__)); \
            if (errorFlags.has(::my::diag::FailureAction::DebugBreak) && ::my::debug::isRunningUnderDebugger())                                                    \
            {                                                                                                                                                      \
                MY_PLATFORM_BREAK;                                                                                                                                 \
            }                                                                                                                                                      \
            if (errorFlags.has(::my::diag::FailureAction::Abort))                                                                                                  \
            {                                                                                                                                                      \
                MY_PLATFORM_ABORT;                                                                                                                                 \
            }                                                                                                                                                      \
        }                                                                                                                                                          \
    }                                                                                                                                                              \
    while (false)

#define MY_FAILURE_IMPL(kind, ...)                                                                                                                            \
    do                                                                                                                                                        \
    {                                                                                                                                                         \
        const auto errorFlags = ::my::diag_detail::raiseFailure(kind, MY_INLINED_SOURCE_INFO, "Failure", ::my::diag_detail::makeFailureMessage(__VA_ARGS__)); \
        if (errorFlags.has(::my::diag::FailureAction::DebugBreak) && ::my::debug::isRunningUnderDebugger())                                                   \
        {                                                                                                                                                     \
            MY_PLATFORM_BREAK;                                                                                                                                \
        }                                                                                                                                                     \
        if (errorFlags.has(::my::diag::FailureAction::Abort))                                                                                                 \
        {                                                                                                                                                     \
            MY_PLATFORM_ABORT;                                                                                                                                \
        }                                                                                                                                                     \
    }                                                                                                                                                         \
    while (false)

#if MY_DEBUG_ASSERT_ENABLED
    #define MY_DEBUG_ASSERT(condition, ...) MY_ASSERT_IMPL(::my::diag::AssertionKind::Default, condition, ##__VA_ARGS__)
    #define MY_DEBUG_FATAL(condition, ...) MY_ASSERT_IMPL(::my::diag::AssertionKind::Fatal, condition, ##__VA_ARGS__)
    #define MY_DEBUG_FAILURE(...) MY_FAILURE_IMPL(::my::diag::AssertionKind::Default, ##__VA_ARGS__)
    #define MY_DEBUG_FATAL_FAILURE(...) MY_FAILURE_IMPL(::my::diag::AssertionKind::Fatal, ##__VA_ARGS__)
#else
    #define MY_DEBUG_ASSERT(condition, ...)
    #define MY_DEBUG_FATAL(condition, ...)
    #define MY_DEBUG_FAILURE(...)
    #define MY_DEBUG_FATAL_FAILURE(...)
#endif

#define MY_ASSERT(condition, ...) MY_ASSERT_IMPL(::my::diag::AssertionKind::Default, condition, ##__VA_ARGS__)
#define MY_FATAL(condition, ...) MY_ASSERT_IMPL(::my::diag::AssertionKind::Fatal, condition, ##__VA_ARGS__)

#define MY_FAILURE(...) MY_FAILURE_IMPL(::my::diag::AssertionKind::Default, ##__VA_ARGS__)
#define MY_FATAL_FAILURE(...) MY_FAILURE_IMPL(::my::diag::AssertionKind::Fatal, ##__VA_ARGS__)
