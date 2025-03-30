// #my_engine_source_file
// nau/diag/log_subscribers.h


#pragma once

#include "my/diag/logging.h"
#include "my/kernel/kernel_config.h"

namespace my::diag
{
    MY_KERNEL_EXPORT ILogSubscriber::Ptr createDebugOutputLogSubscriber();

    MY_KERNEL_EXPORT ILogSubscriber::Ptr createConioOutputLogSubscriber();

    MY_KERNEL_EXPORT ILogSubscriber::Ptr createFileOutputLogSubscriber(std::string_view filename);

}  // namespace my::diag