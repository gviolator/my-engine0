// #my_engine_source_file
#pragma once
#include "lua_internals.h"
#include "lua_toolkit/lua_interop.h"
#include "lua_toolkit/lua_utils.h"
#include "my/rtti/rtti_impl.h"
#include "value_roots.h"

namespace my::lua_detail
{
   /**
          Stores a reference to the parent and the key by which the parent can find the given value.
          The object itself "does not know" how its value can be placed on the lua stack - parent + key is responsible for this.
      */
    class MY_ABSTRACT_TYPE ReferenceValue 
    {
        // MY_INTERFACE(my::lua_detail::ReferenceValue, IRefCounted)
        MY_TYPEID(my::lua_detail::ReferenceValue);

    public:
        ReferenceValue(LuaRootPtr root, lua::ChildVariableKey key);
        ReferenceValue(const ReferenceValue&) = delete;
        ~ReferenceValue();
        ReferenceValue operator=(const ReferenceValue&) = delete;

        lua_State* getLua() const;

        int pushSelf() const;

    protected:
        const LuaRootPtr m_root;
        const lua::ChildVariableKey m_key;
    };

}  // namespace my::lua_detail