// #my_engine_source_file

#include "lua_internals.h"
#include "my/utils/string_utils.h"

namespace my::lua {

ChildVariableKey ChildVariableKey::MakeFromStack(lua_State* l, int index)
{
    const int keyType = lua_type(l, index);
    MY_DEBUG_ASSERT(keyType == LUA_TNUMBER || keyType == LUA_TSTRING);

    if (keyType == LUA_TNUMBER)
    {
        return static_cast<int>(lua_tointeger(l, index));
    }

    size_t len;
    const char* const value = lua_tolstring(l, index, &len);
    return std::string_view{value, len};
}

std::string_view ChildVariableKey::AsString() const
{
    if (!*this)
    {
        return {};
    }

    if (m_index != InvalidLuaIndex)
    {
        if (m_name.empty())
        {
            m_name = strings::lexicalCast(m_index);
        }
    }

    return m_name;
}

// int ChildVariableKey::asIndex() const
// {
//     if(!*this)
//     {
//         return {};
//     }

//     if(m_index != InvalidLuaIndex)
//     {
//         if(m_name.empty())
//         {
//             m_name = strings::lexicalCast(m_index);
//         }
//     }

//     return m_name;
// }

bool ChildVariableKey::operator==(std::string_view str) const
{
    MY_DEBUG_ASSERT(static_cast<bool>(*this));

    if (IsIndexed())
    {
        return std::to_string(m_index) == str;
    }

    return m_name == str;
}

void ChildVariableKey::Push(lua_State* l) const
{
    MY_DEBUG_ASSERT(static_cast<bool>(*this));

    if (m_index != InvalidLuaIndex)
    {
        lua_pushinteger(l, static_cast<lua_Integer>(m_index));
    }
    else
    {
        lua_pushlstring(l, m_name.c_str(), m_name.size());
    }
}

}  // namespace my::lua
