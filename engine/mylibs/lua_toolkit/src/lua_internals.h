// #my_engine_source_file

#include <limits>
#include <string_view>

#include "lua_toolkit/lua_headers.h"
#include "my/diag/assert.h"

namespace my::lua
{
    inline constexpr int InvalidLuaIndex = std::numeric_limits<int>::min();

    class ChildVariableKey
    {
    public:
        static ChildVariableKey makeFromStack(lua_State*, int);

        static inline ChildVariableKey noKey()
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

        bool isIndexed() const
        {
            return m_index != InvalidLuaIndex;
        }

        explicit operator bool() const
        {
            return isIndexed() || !m_name.empty();
        }

        operator int() const
        {
            MY_DEBUG_ASSERT(isIndexed());
            return m_index;
        }

        const std::string& getName() const
        {
            MY_DEBUG_ASSERT(!isIndexed());
            MY_DEBUG_ASSERT(m_name.length() > 0);
            return (m_name);
        }

        std::string_view asString() const;

        bool operator==(const ChildVariableKey& other) const
        {
            MY_DEBUG_ASSERT(static_cast<bool>(*this));
            MY_DEBUG_ASSERT(static_cast<bool>(other));

            if (other.isIndexed() != this->isIndexed())
            {
                return false;
            }

            return isIndexed() ? this->m_index == other.m_index : this->m_name == other.m_name;
        }

        bool operator!=(const ChildVariableKey& other) const
        {
            return !this->operator==(other);
        }

        bool operator==(std::string_view) const;

        void push(lua_State*) const;

    private:
        int m_index = InvalidLuaIndex;
        mutable std::string m_name;
    };

}  // namespace my::lua
