// #my_engine_source_file
#pragma once
#include <uv.h>

#include <string>

#include "my/diag/assert.h"

namespace my
{
    std::string getUVErrorMessage(int code);
}

#define UV_VERIFY(expression)                                                                                                                      \
    do                                                                                                                                             \
    {                                                                                                                                              \
        if (const int errCode = expression; errCode != 0) [[unlikely]]                                                                             \
        {                                                                                                                                          \
            const std::string message = my::getUVErrorMessage(errCode);                                                                            \
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
    } while (0)
