// #my_engine_source_file
#pragma once

#include <string>
#include <string_view>

#include "my/kernel/kernel_config.h"

namespace my::strings
{
    MY_KERNEL_EXPORT std::wstring utf8ToWString(std::string_view text);

    MY_KERNEL_EXPORT std::string wstringToUtf8(std::wstring_view text);

}  // namespace my::strings
