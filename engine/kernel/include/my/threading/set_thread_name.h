// #my_engine_source_header
// nau/threading/set_thread_name.h


#pragma once
#include <string>

#include "my/kernel/kernel_config.h"

namespace my::threading
{
    MY_KERNEL_EXPORT void setThisThreadName(const std::string& name);

}  // namespace my::threading
