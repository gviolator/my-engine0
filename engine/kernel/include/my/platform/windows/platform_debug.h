// #my_engine_source_header


#pragma once

#include <cstdlib>
#include <intrin.h>

#define MY_PLATFORM_BREAK (__nop(), __debugbreak())
#define MY_PLATFORM_ABORT (std::abort())
