// #my_engine_source_file
#pragma once
#include "my/diag/logging_base.h"
#include "my/kernel/kernel_config.h"

namespace my::diag
{

    struct MY_ABSTRACT_TYPE LogLevelFilter : LogFilter
    {
        MY_INTERFACE(LogLevelFilter, LogFilter)

        virtual void setLevel(LogLevel level) = 0;
    };
}  // namespace my::diag