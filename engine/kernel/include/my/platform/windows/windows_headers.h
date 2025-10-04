// #my_engine_source_file

#pragma once

#if defined(_INC_WINDOWS) || defined(_WINDOWS_)
    #error Windows.h already included
#endif

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
    #define NOMINMAX
#endif

#include <Windows.h>
#include <WinSock2.h>

#ifndef __FUNCTION_NAME__
    #ifdef WIN32  // WINDOWS
        #define __FUNCTION_NAME__ __FUNCTION__
    #else  //*NIX
        #define __FUNCTION_NAME__ __func__
    #endif
#endif
