// #my_engine_source_file

#pragma once

#if defined(_MSC_VER)
    #if !__cpp_impl_coroutine
        #error "Require coroutine implementation"
    #endif

    #include <coroutine>

#elif __linux__
    #include <coroutine>
#else
    #error Setup coroutine

#endif
