// #my_engine_source_file

#pragma once

#include "my/diag/logging_base.h"
#include "my/kernel/kernel_config.h"
#include "my/io/stream.h"

namespace my::diag
{
    MY_KERNEL_EXPORT io::StreamPtr createDebugOutputStream();

    MY_KERNEL_EXPORT LogSinkPtr createPlainTextSink(io::StreamPtr stream);
    MY_KERNEL_EXPORT LogSinkPtr createHtmlTextSink(io::StreamPtr stream);
    MY_KERNEL_EXPORT LogSinkPtr createConsoleSink(io::StreamPtr outStream = nullptr, io::StreamPtr errorStream = nullptr);

}  // namespace my::diag
