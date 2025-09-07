// #my_engine_source_file

#pragma once

#ifdef MY_PLATFORM_WINDOWS
    #include "my/platform/windows/windows_headers.h"
#endif

#include <atomic>
#include <chrono>
#include <cmath>
#include <forward_list>
#include <functional>
#include <future>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <random>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <vector>

extern "C"
{
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#ifdef Yield
    #undef Yield
#endif

#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdeprecated-copy"
#endif

#ifdef __clang__
    #pragma clang diagnostic pop
#endif


#include "my/diag/logging.h"

