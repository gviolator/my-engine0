// #my_engine_source_file
#pragma once

#include "my/diag/assert.h"

#include <limits>
#include <string>
#include <string_view>

// #include "lua_toolkit/lua_headers.h"

namespace my::lua {

inline constexpr int InvalidLuaIndex = std::numeric_limits<int>::min();

class ChildVariableKey
{
public:
    static ChildVariableKey MakeFromStack(lua_State*, int);

    static inline ChildVariableKey NoKey()
    {
        return nullptr;
    }

    ChildVariableKey() = default;

    ChildVariableKey(std::nullptr_t) :
        ChildVariableKey()
    {
    }

    ChildVariableKey(int indexedKey) :
        m_index(indexedKey)
    {
    }

    ChildVariableKey(std::string_view namedKey) :
        m_name(namedKey)
    {
    }

    bool IsIndexed() const
    {
        return m_index != InvalidLuaIndex;
    }

    explicit operator bool() const
    {
        return IsIndexed() || !m_name.empty();
    }

    operator int() const
    {
        MY_DEBUG_ASSERT(IsIndexed());
        return m_index;
    }

    const std::string& GetName() const
    {
        MY_DEBUG_ASSERT(!IsIndexed());
        MY_DEBUG_ASSERT(m_name.length() > 0);
        return (m_name);
    }

    std::string_view AsString() const;

    bool operator==(const ChildVariableKey& other) const
    {
        MY_DEBUG_ASSERT(static_cast<bool>(*this));
        MY_DEBUG_ASSERT(static_cast<bool>(other));

        if (other.IsIndexed() != this->IsIndexed())
        {
            return false;
        }

        return IsIndexed() ? this->m_index == other.m_index : this->m_name == other.m_name;
    }

    bool operator!=(const ChildVariableKey& other) const
    {
        return !this->operator==(other);
    }

    bool operator==(std::string_view) const;

    void Push(lua_State*) const;

private:
    int m_index = InvalidLuaIndex;
    mutable std::string m_name;
};

}  // namespace my::lua
