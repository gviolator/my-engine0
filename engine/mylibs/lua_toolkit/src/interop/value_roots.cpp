// #my_engine_source_file
#include "value_roots.h"

#include "lua_toolkit/lua_utils.h"
#include "my/rtti/weak_ptr.h"

namespace my::lua_detail
{
    namespace
    {
        template <typename T>
        Ptr<T> getRootThreadLocalInstance(lua_State* l)
        {
            using RootMap = std::unordered_map<lua_State*, WeakPtr<T>>;

            static thread_local RootMap roots;

            WeakPtr<T>& ref = roots[l];
            Ptr<T> instance = ref.acquire();
            if (!instance)
            {
                instance = rtti::createInstance<T>(l);
                ref = instance;
            }

            MY_DEBUG_FATAL(instance && instance->getLua() == l);
            return instance;
        }

        inline const char* GlobalRefsFieldName = "__My_GlobalRefs";

    }  // namespace

    Ptr<LuaRoot> LuaStackRoot::instance(lua_State* l)
    {
        return getRootThreadLocalInstance<LuaStackRoot>(l);
    }

    LuaStackRoot::LuaStackRoot(lua_State* l) :
        LuaRoot(l)
    {
    }

    int LuaStackRoot::push(const lua::ChildVariableKey& key) const
    {
        MY_DEBUG_ASSERT(key && key.isIndexed());
        lua_pushvalue(m_lua, key);
        return lua_gettop(m_lua);
    }

    void LuaStackRoot::unref(const lua::ChildVariableKey&)
    {
    }

    lua::ChildVariableKey LuaStackRoot::ref(int index)
    {
        return lua::getAbsoluteStackPos(m_lua, index);
    }

    /**
     */
    Ptr<LuaRoot> LuaGlobalRefRoot::instance(lua_State* l)
    {
        return getRootThreadLocalInstance<LuaGlobalRefRoot>(l);
    }

    LuaGlobalRefRoot::LuaGlobalRefRoot(lua_State* l) :
        LuaRoot(l)
    {
        assert_lstack_unchanged(m_lua);

#if MY_DEBUG_ASSERT_ENABLED
        MY_DEBUG_ASSERT(lua_getglobal(m_lua, GlobalRefsFieldName) == LUA_TNIL);
        lua_pop(m_lua, 1);
#endif
        lua_createtable(m_lua, 0, 0);
        lua_setglobal(m_lua, GlobalRefsFieldName);
    }

    LuaGlobalRefRoot::~LuaGlobalRefRoot()
    {
        lua_pushnil(m_lua);
        lua_setglobal(m_lua, GlobalRefsFieldName);
    }

    int LuaGlobalRefRoot::push(const lua::ChildVariableKey& key) const
    {
        MY_DEBUG_ASSERT(key && key.isIndexed());

        {
            [[maybe_unused]] const auto t = lua_getglobal(m_lua, GlobalRefsFieldName);
            MY_DEBUG_ASSERT(t == LUA_TTABLE);
        }

        {
            [[maybe_unused]] const int t = lua_rawgeti(m_lua, -1, key);
            MY_DEBUG_ASSERT(t != LUA_TNIL, "Invalid reference index ({})", key.asString());
            lua_remove(m_lua, -2);
        }

        return lua_gettop(m_lua);
    }

    void LuaGlobalRefRoot::unref(const lua::ChildVariableKey& key)
    {
        MY_DEBUG_ASSERT(key && key.isIndexed());
        if (!key || !key.isIndexed())
        {
            return;
        }
    }

    lua::ChildVariableKey LuaGlobalRefRoot::ref(int idx)
    {
        assert_lstack_unchanged(m_lua);

        const auto absIndex = lua::getAbsoluteStackPos(m_lua, idx);

        [[maybe_unused]] const auto t = lua_getglobal(m_lua, GlobalRefsFieldName);
        MY_DEBUG_ASSERT(t == LUA_TTABLE);

        lua_pushvalue(m_lua, absIndex);
        // make ref to value on top on stack and pop it:
        // ref_id = lau make new ref id;
        // i.e. __My_GlobalRefs[ref_id] = value_on_stack_top
        // pop value from stack
        const int refId = luaL_ref(m_lua, -2);
        MY_DEBUG_ASSERT(refId > 0);

        // pop My_GlobalRefs table
        lua_pop(m_lua, 1);
        return refId;
    }

}  // namespace my::lua_detail
