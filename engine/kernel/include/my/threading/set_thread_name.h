// #my_engine_source_file

#pragma once
#include "my/kernel/kernel_config.h"

#include <string>


namespace my::threading {

MY_KERNEL_EXPORT void setThisThreadName(const std::string& name);

}  // namespace my::threading
