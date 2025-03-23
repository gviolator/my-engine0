// #my_engine_source_header


#pragma once

#if defined(_MSC_VER)
#if !__cpp_impl_coroutine
#error "Require coroutine implementation"
#endif

#include <coroutine>

//namespace CoroNs = std;

#else

#error Setup coroutine

#endif



