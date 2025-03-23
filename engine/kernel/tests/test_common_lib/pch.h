// #my_engine_source_header


#pragma once

#ifdef MY_PLATFORM_WINDOWS
    #include "my/platform/windows/windows_headers.h"
#endif

#ifdef Yield
    #undef Yield
#endif

#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdeprecated-copy"
#endif

#include <gmock/gmock.h>
#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

#ifdef __clang__
    #pragma clang diagnostic pop
#endif
