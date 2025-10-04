// #my_engine_source_file
#pragma once

#include <uv.h>

#include <string_view>

namespace my {
/**
 */
std::string_view getUVErrorMessage(int code);
}  // namespace my

#define UV_VERIFY(expression)                                                                                                                      \
    do                                                                                                                                             \
    {                                                                                                                                              \
        if (const int errCode = expression; errCode != 0) [[unlikely]]                                                                             \
        {                                                                                                                                          \
            const std::string_view message = my::getUVErrorMessage(errCode);                                                                       \
            const auto errorFlags = ::my::diag_detail::raiseFailure(my::diag::AssertionKind::Fatal, MY_INLINED_SOURCE_INFO, #expression, message); \
            if (errorFlags.has(::my::diag::FailureAction::DebugBreak) && ::my::debug::isRunningUnderDebugger())                                    \
            {                                                                                                                                      \
                MY_PLATFORM_BREAK;                                                                                                                 \
            }                                                                                                                                      \
            if (errorFlags.has(::my::diag::FailureAction::Abort))                                                                                  \
            {                                                                                                                                      \
                MY_PLATFORM_ABORT;                                                                                                                 \
            }                                                                                                                                      \
        }                                                                                                                                          \
    }                                                                                                                                              \
    while (0)
