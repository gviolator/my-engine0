// #my_engine_source_file

#pragma once

#if !__has_include(<lua.h>)
    #error lua not configured to be used with current project
#endif

extern "C"
{
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}
