// #my_engine_source_file


#pragma once

#define MY_CONCAT_IMPL(s1, s2) s1##s2
#define PP_CONCATENATE(s1, s2) MY_CONCAT_IMPL(s1, s2)

#define PP_STRINGIZE(M) #M
#define PP_STRINGIZE_VALUE(M) PP_STRINGIZE(M)

#ifdef __COUNTER__
    #define ANONYMOUS_VAR(Prefix) PP_CONCATENATE(Prefix, __COUNTER__)
#else
    #define ANONYMOUS_VAR(Prefix) PP_CONCATENATE(Prefix, _LINE__)
#endif

// #define WFILE CONCATENATE(L, __FILE__)
// #define WFUNCTION CONCATENATE(L, __FUNCTION__)

//

#if defined(_MSC_VER) && !defined(__clang__)
    #if defined(_MSVC_TRADITIONAL) && _MSVC_TRADITIONAL
        #define PP_VA_OPT_AVAILABLE 0
    #else
        #define PP_VA_OPT_AVAILABLE 1
    #endif

#elif defined(__clang__)

    #define PP_VA_OPT_AVAILABLE 1

#endif
/*
#if !defined(PP_VA_OPT_AVAILABLE) || !PP_VA_OPT_AVAILABLE
#error setup for other compilers
#endif
*/

#ifdef _WIN32
#define MY_PLATFORM_HEADER_DIR_NAME windows
#elif defined(__linux__)
#define MY_PLATFORM_HEADER_DIR_NAME linux_os
#endif

#define MY_PLATFORM_PATH_IMPL(Platform, FileName) my/platform/Platform/FileName
#define MY_PLATFORM_PATH(FileName) MY_PLATFORM_PATH_IMPL(MY_PLATFORM_HEADER_DIR_NAME, FileName)

#define MY_PLATFORM_HEADER(FileName) PP_STRINGIZE_VALUE(MY_PLATFORM_PATH(FileName))
