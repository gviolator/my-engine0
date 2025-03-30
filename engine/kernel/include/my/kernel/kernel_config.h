// #my_engine_source_file


#pragma once

#if !defined(MY_STATIC_RUNTIME)
    #ifdef _MSC_VER
        #ifdef MY_KERNEL_BUILD
            #define MY_KERNEL_EXPORT __declspec(dllexport)
        #else
            #define MY_KERNEL_EXPORT __declspec(dllimport)
        #endif

    #else
        #error Unknown Compiler/OS
    #endif

#else
    #define MY_KERNEL_EXPORT
#endif

#if defined(__clang__)
    #if __has_feature(cxx_rtti)
        #define MY_CXX_RTTI
    #endif
#elif defined(__GNUC__)
    #if defined(__GXX_RTTI)
        #define MY_CXX_RTTI
    #endif
#elif defined(_MSC_VER)
    #if defined(_CPPRTTI)
        #define MY_CXX_RTTI
    #endif
#endif

#ifndef NDEBUG
    #ifndef _DEBUG
        #define _DEBUG 1
    #endif
    #ifndef DEBUG
        #define DEBUG 1
    #endif
#endif

#if defined(_DEBUG) || !defined(NDEBUG) || DEBUG
    #define MY_DEBUG 1
#else
    #define MY_DEBUG 0
#endif

#if !defined(MY_DEBUG_CHECK_ENABLED)
    #ifdef _DEBUG
        #define MY_DEBUG_CHECK_ENABLED 1
    #else
        #define MY_DEBUG_CHECK_ENABLED 0
    #endif
#endif

#ifndef MY_NOINLINE
    #if defined(__GNUC__)
        #define MY_NOINLINE __attribute__((noinline))
    #elif _MSC_VER >= 1300
        #define MY_NOINLINE __declspec(noinline)
    #else
        #define MY_NOINLINE
    #endif
#endif

// #ifndef MY_LIKELY
//     #if(defined(__GNUC__) && (__GNUC__ >= 3)) || defined(__clang__)
//         #if defined(__cplusplus)
//             #define MY_LIKELY(x) __builtin_expect(!!(x), true)
//             #define MY_UNLIKELY(x) __builtin_expect(!!(x), false)
//         #else
//             #define MY_LIKELY(x) __builtin_expect(!!(x), 1)
//             #define MY_UNLIKELY(x) __builtin_expect(!!(x), 0)
//         #endif
//     #else
//         #define MY_LIKELY(x) (x)
//         #define MY_UNLIKELY(x) (x)
//     #endif
// #endif

// #ifndef MY_THREAD_SANITIZER
//     #if defined(__has_feature)
//         #if __has_feature(thread_sanitizer)
//             #define MY_THREAD_SANITIZER 1
//         #endif
//     #else
//         #if defined(__SANITIZE_THREAD__)
//             #define MY_THREAD_SANITIZER 1
//         #endif
//     #endif
// #endif

// #ifndef MY_ADDRESS_SANITIZER
//     #if defined(__has_feature)
//         #if __has_feature(address_sanitizer)
//             #define MY_ADDRESS_SANITIZER 1
//         #endif
//     #else
//         #if defined(__SANITIZE_ADDRESS__)
//             #define MY_ADDRESS_SANITIZER 1
//         #endif
//     #endif
// #endif

 #if !defined(MY_FORCE_INLINE)
     #define MY_FORCE_INLINE __forceinline
 #endif

// #if !defined(MY_UNUSED)
//     #define MY_UNUSED(x) ((void)(x))
// #endif

// #ifdef __analysis_assume
//     #define MY_ANALYSIS_ASSUME __analysis_assume
// #else
//     #define MY_ANALYSIS_ASSUME(expr)
// #endif

#define MY_THREADS_ENABLED

// #if _WIN64 || defined(__LP64__)
//     #define _TARGET_64_BIT 1
// #endif  //  _WIN64 || defined(__LP64__)
