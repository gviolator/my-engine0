// #my_engine_source_file

#pragma once

#include <optional>

#include "my/async/executor.h"
#include "my/kernel/kernel_config.h"

namespace my::async
{
    MY_KERNEL_EXPORT ExecutorPtr createThreadPoolExecutor(std::optional<size_t> threadsCount = std::nullopt);
}  // namespace my::async
