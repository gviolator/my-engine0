// #my_engine_source_header

#pragma once

#ifdef MY_PLATFORM_WINDOWS
    #include "my/platform/windows/windows_headers.h"
#endif

#include <algorithm>
#include <bit>
#include <filesystem>
#include <format>
#include <iostream>
#include <mutex>
#include <optional>
#include <regex>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
