// #my_engine_source_file
#include "lua_debugger_state.h"
#include "lua_toolkit/lua_utils.h"
#include "my/diag/assert.h"
#include "my/utils/scope_guard.h"

namespace my::lua {

bool DebuggerState::IsStateExists(lua_State* l)
{
    scope_on_leave
    {
        lua_pop(l, 1);
    };

    lua_getglobal(l, DebuggerState::kFieldName_DebuggerState);
    return lua_type(l, -1) == LUA_TNIL;
}

void DebuggerState::InstallState(lua_State* l, void* dataPtr, GetArCallback arCallback)
{
    MY_DEBUG_ASSERT(IsStateExists(l));

    lua_createtable(l, 0, 2);
    lua_pushvalue(l, -1);
    lua_setglobal(l, kFieldName_DebuggerState);  // FieldName_DebugSession table top on stack

    lua_pushlightuserdata(l, dataPtr);
    lua_setfield(l, -2, "dataPtr");

    // __my_DebugSession.refs = {};
    lua_createtable(l, 0, 0);
    lua_setfield(l, -2, "refs");

    // __my_DebugSession.GetStackLocal = function + closure upvalues
    lua_pushlightuserdata(l, reinterpret_cast<void*>(arCallback));
    lua_pushlightuserdata(l, dataPtr);

    lua_pushcclosure(l, [](lua_State* l) noexcept -> int
    {
        MY_DEBUG_ASSERT(lua_type(l, lua_upvalueindex(1)) == LUA_TLIGHTUSERDATA);
        MY_DEBUG_ASSERT(lua_type(l, lua_upvalueindex(2)) == LUA_TLIGHTUSERDATA);

        GetArCallback arCallback = reinterpret_cast<GetArCallback>(lua_touserdata(l, lua_upvalueindex(1)));
        lua_Debug* const ar = arCallback(lua_touserdata(l, lua_upvalueindex(2)));

        if (!ar)
        {
            lua_pushstring(l, "No active stack trace");
            lua_error(l);
            return 0;
        }

        MY_DEBUG_ASSERT(lua_gettop(l) == 2);
        MY_DEBUG_ASSERT(lua_type(l, -2) == LUA_TNUMBER);
        MY_DEBUG_ASSERT(lua_type(l, -1) == LUA_TSTRING);

        // frameLevel - номер фрейма для которого ищем локальное значение (если < 0, то ищем по всему стеку)
        // Необходимо "сместиться" по стеку на два фрейма:
        // 0: [.GetStackLocal]
        // 1: [eval chunk]
        // 2: оригинальный стек.
        constexpr int kStackFrameOffset = 2;
        const int requestedFrame = static_cast<int>(lua_tointeger(l, -2));
        int targetFrameLevel = requestedFrame >= 0 ? requestedFrame + kStackFrameOffset : -1;

        std::string_view targetName = EXPR_Block
        {
            size_t nameLen = 0;
            const char* const namePtr = lua_tolstring(l, -1, &nameLen);
            return std::string_view{namePtr, nameLen};
        };

        MY_DEBUG_ASSERT(!targetName.empty());

        // цикл нужен для случая когда не указан конкретный фрейм и ищется первое совпадение на любом уровне стека.
        for (int level = targetFrameLevel < 0 ? kStackFrameOffset : targetFrameLevel; lua_getstack(l, level, ar) != 0; ++level)
        {
            for (std::string_view name : lua::StackLocalsEnumerator{l, ar})
            {
                if (targetName == name)
                {
                    // значение локальной переменной находится на стеке (lua_pop вызывается внутри iterator::operator++);
                    return true;
                }
            }

            if (lua_getinfo(l, "fu", ar) != 0)
            {
                for (std::string_view name : lua::UpValuesEnumerator{l})
                {
                    if (targetName == name)
                    {
                        // значение upvalue переменной находится на стеке (lua_pop вызывается внутри iterator::operator++);
                        return true;
                    }
                }
            }

            if (level == targetFrameLevel)
            {
                break;
            }
        }

        const auto message = fmt::format("Local ({}) unresolved", targetName);
        lua_pushlstring(l, message.c_str(), message.length());
        lua_error(l);
        return 1;
    }, 2);

    lua_setfield(l, -2, "GetStackLocal");
}

void DebuggerState::ReleaseState(lua_State* l)
{
    lua_pushnil(l);
    // lua_setfield(l, LUA_GLOBALSINDEX, kFieldName_DebugSession);
    lua_setglobal(l, kFieldName_DebuggerState);
}

void* DebuggerState::GetClientPtr(lua_State* l)
{
    guard_lstack(l);

    // lua_getfield(l, LUA_GLOBALSINDEX, kFieldName_DebugSession);
    lua_getglobal(l, kFieldName_DebuggerState);
    if (lua_type(l, -1) != LUA_TTABLE)
    {
        return nullptr;
    }

    lua_getfield(l, -1, "dataPtr");
    MY_DEBUG_ASSERT(lua_type(l, -1) == LUA_TLIGHTUSERDATA);

    return lua_type(l, -1) == LUA_TLIGHTUSERDATA ? lua_touserdata(l, -1) : nullptr;
}

int DebuggerState::KeepReference(lua_State* l)
{
    MY_DEBUG_ASSERT(lua_gettop(l) > 0);
    MY_DEBUG_ASSERT(!(lua_type(l, -1) == LUA_TNIL || lua_type(l, -1) == LUA_TNONE), "Can not keep reference to nothing");

    lua_getglobal(l, kFieldName_DebuggerState);
    MY_DEBUG_ASSERT(lua_type(l, -1) == LUA_TTABLE);

    lua_getfield(l, -1, "refs");
    MY_DEBUG_ASSERT(lua_type(l, -1) == LUA_TTABLE);

    // -3 referenced value
    // -2 FieldName_DebugSession
    // -1 refs

    lua_pushvalue(l, -3);

    // -4 referenced value
    // -3 FieldName_DebugSession
    // -2 refs
    // -1 referenced value
    const int refId = luaL_ref(l, -2);  // luaL_ref pop  -1 referenced value

    lua_pop(l, 3);  // pop: refs, FieldName_DebugSession, original value

    return refId;
}

void DebuggerState::ReleaseReference(lua_State* l, int refId)
{
    guard_lstack(l);

    lua_getglobal(l, kFieldName_DebuggerState);
    MY_DEBUG_ASSERT(lua_type(l, -1) == LUA_TTABLE);

    lua_getfield(l, -1, "refs");
    MY_DEBUG_ASSERT(lua_type(l, -1) == LUA_TTABLE);

    luaL_unref(l, -1, refId);
}

void DebuggerState::GetReference(lua_State* l, int refId)
{
    lua_getglobal(l, kFieldName_DebuggerState);
    MY_DEBUG_ASSERT(lua_type(l, -1) == LUA_TTABLE);

    lua_getfield(l, -1, "refs");
    MY_DEBUG_ASSERT(lua_type(l, -1) == LUA_TTABLE);

    lua_rawgeti(l, -1, refId);

    lua_remove(l, -2);  // "refs"
    lua_remove(l, -2);  // FieldName_DebugSession
}
}  // namespace my::lua
