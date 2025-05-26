// #my_engine_source_file

#if !defined(MY_STATIC_RUNTIME)

    #ifdef _MSC_VER
        #ifdef MY_LUATOOLKIT_BUILD
            #define MY_LUATOOLKIT_EXPORT __declspec(dllexport)
        #else
            #define MY_LUATOOLKIT_EXPORT __declspec(dllimport)
        #endif

    #else
        #error Unknown Compiler/OS
    #endif

#else
    #define MY_LUATOOLKIT_EXPORT
#endif
