// #my_engine_source_file
#pragma once

#include "lua_internals.h"
#include "my/rtti/ptr.h"
#include "my/rtti/rtti_impl.h"
#include "my/rtti/rtti_object.h"

namespace my::lua_detail
{
    struct MY_ABSTRACT_TYPE LuaRoot : virtual IRefCounted
    {
        MY_INTERFACE(my::lua_detail::LuaRoot, IRefCounted)

        lua_State* getLua() const
        {
            return m_lua;
        }

        virtual int push(const lua::ChildVariableKey&) const = 0;

        virtual lua::ChildVariableKey ref(int idx) = 0;

        virtual void unref(const lua::ChildVariableKey&) = 0;

    protected:
        LuaRoot(lua_State* l) :
            m_lua(l)
        {
            MY_DEBUG_FATAL(m_lua);
        }

        lua_State* const m_lua;
    };

    using LuaRootPtr = Ptr<LuaRoot>;

    /**
     */
    class LuaStackRoot final : public LuaRoot
    {
        MY_REFCOUNTED_CLASS(LuaStackRoot, LuaRoot)

    public:
        static Ptr<LuaRoot> instance(lua_State* l);

        LuaStackRoot(lua_State* l);

    private:
        int push(const lua::ChildVariableKey& key) const override;
        lua::ChildVariableKey ref(int idx) override;
        void unref(const lua::ChildVariableKey&) override;
    };

    /**
     */
    class LuaGlobalRefRoot final : public LuaRoot
    {
        MY_REFCOUNTED_CLASS(LuaGlobalRefRoot, LuaRoot)

    public:
        static Ptr<LuaRoot> instance(lua_State* l);

        LuaGlobalRefRoot(lua_State* l);

        ~LuaGlobalRefRoot();

    private:
        int push(const lua::ChildVariableKey& key) const override;
        lua::ChildVariableKey ref(int idx) override;
        void unref(const lua::ChildVariableKey&) override;
    };
}  // namespace my::lua_detail