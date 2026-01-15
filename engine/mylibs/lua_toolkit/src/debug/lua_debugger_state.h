// #my_engine_source_file
#pragma once

#include "lua_toolkit/lua_header.h"

namespace my::lua {

struct DebuggerState
{
    static inline constexpr const char* const kFieldName_DebuggerState = "__my_DebugSession";

    using GetArCallback = lua_Debug* (*)(void*) noexcept;

    /*
        ������������� ��������� ���������� ������:
            __my_DebugSession = {}
            __my_DebugSession .dataPtr = dataPtr;
            __my_DebugSession .arCallback = arCallback;
            __my_DebugSession .refs = {};
    */
    static void InstallState(lua_State*, void* clientPtr, GetArCallback arCallback);

    static void ReleaseState(lua_State*);

    static bool IsStateExists(lua_State*);

    static void* GetClientPtr(lua_State*);

    /*
        ����� ������ �� �������� �� ������� �����.
        �������� ��������� �� �����.
        ���������� ������������� ������, ������� ������ ���� ����������� � ReleaseReference.
    */
    static int KeepReference(lua_State*);

    /*
     */
    static void ReleaseReference(lua_State*, int refId);

    /*
        ����� ��������, �� ����.
        � ������ ���������� ������ �� ����� ����� nil.
    */
    static void GetReference(lua_State*, int refId);


};
}  // namespace my::lua
