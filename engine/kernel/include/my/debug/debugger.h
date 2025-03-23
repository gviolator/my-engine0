// #my_engine_source_header
#pragma once

#include "my/kernel/kernel_config.h"
#include "my/utils/preprocessor.h"

#include MY_PLATFORM_HEADER(platform_debug.h)

namespace my::debug
{
    MY_KERNEL_EXPORT bool isRunningUnderDebugger();

    inline void debugBreak()
    {
        MY_PLATFORM_BREAK;
    }

} // namespace my::debug
