// #my_engine_source_file

#pragma once
#include "lua_toolkit/lua_header.h"
#include "lua_toolkit/lua_toolkit_config.h"
#include "my/diag/assert.h"
#include "my/io/stream.h"
#include "my/utils/preprocessor.h"
#include "my/utils/result.h"
#include "my/utils/scope_guard.h"

#include <span>
#include <string_view>

namespace my::lua {
/**
 */
struct MY_LUATOOLKIT_EXPORT StackGuard
{
    lua_State* const luaState;
    const int top;

    StackGuard(lua_State* l);
    ~StackGuard();

    StackGuard(const StackGuard&) = delete;
    StackGuard& operator=(const StackGuard&) = delete;
};

#if MY_DEBUG_ASSERT_ENABLED
struct StackNotChangesAssertionGuard
{
    lua_State* const luaState;
    const int top;

    StackNotChangesAssertionGuard(lua_State* l) :
        luaState(l),
        top(lua_gettop(l))
    {
    }

    ~StackNotChangesAssertionGuard() noexcept
    {
        const int currentTop = lua_gettop(luaState);
        MY_DEBUG_ASSERT(currentTop == top, "Lua's stack expected to be unchanged ({}), but ({})", top, currentTop);
    }
};
#else
struct StackNotChangesAssertionGuard
{
    StackNotChangesAssertionGuard([[maybe_unused]] lua_State*)
    {
    }
};
#endif

/**
 */
class MY_LUATOOLKIT_EXPORT GlobalReference
{
public:
    GlobalReference();
    GlobalReference(lua_State*);
    GlobalReference(lua_State*, int stackIndex);
    GlobalReference(const GlobalReference&) = delete;
    GlobalReference(GlobalReference&&);

    GlobalReference& operator=(const GlobalReference&) = delete;
    GlobalReference& operator=(GlobalReference&&);
    GlobalReference& operator=(std::nullptr_t);

    int getRef() const;
    void push() const;
    void reset();

private:
    lua_State* m_lua = nullptr;
    int m_ref = LUA_NOREF;
};

/**
 */
class UpValuesEnumerator
{
public:
    class iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::string_view;
        using difference_type = size_t;
        using pointer = nullptr_t;
        using reference = nullptr_t;

        iterator();
        iterator& operator++();
        std::string_view operator*() const;
        std::string_view Name() const;
        int Index() const;

    private:
        iterator(lua_State* l, int funcIndex, int upvalIndex);

        lua_State* const m_lua = nullptr;
        const int m_funcIndex = 0;
        int m_upvalIndex = -1;
        const char* m_name = nullptr;

        friend class UpValuesEnumerator;
        friend bool operator==(const iterator&, const iterator&);
        friend bool operator!=(const iterator&, const iterator&);
    };

    UpValuesEnumerator(lua_State*, int stackIndex = -1);
    UpValuesEnumerator(const UpValuesEnumerator&) = delete;
    iterator begin() const;
    iterator end() const;

private:
    lua_State* const m_lua;
    const int m_funcIndex;
};

/**
 */
class StackLocalsEnumerator
{
public:
    class iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::string_view;
        using difference_type = size_t;
        using pointer = nullptr_t;
        using reference = nullptr_t;

        iterator();
        iterator& operator++();
        std::string_view operator*() const;
        std::string_view Name() const;
        int Index() const;

    private:
        iterator(lua_State* l, lua_Debug* ar, int n);

        lua_State* const m_lua = nullptr;
        lua_Debug* const m_activationRec = nullptr;
        int m_index = -1;
        const char* m_name = nullptr;

        friend class StackLocalsEnumerator;
        friend bool operator==(const iterator&, const iterator&);
        friend bool operator!=(const iterator&, const iterator&);
    };

    StackLocalsEnumerator(lua_State*, lua_Debug*);
    StackLocalsEnumerator(const StackLocalsEnumerator&) = delete;
    iterator begin() const;
    iterator end() const;

private:
    lua_State* const m_lua;
    lua_Debug* const m_activationRec;
};
/**
 */
class TableEnumerator
{
public:
    class iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::string_view;
        using difference_type = size_t;
        using pointer = nullptr_t;
        using reference = nullptr_t;

        iterator() = default;
        iterator& operator++();
        constexpr std::pair<int, int> operator*() const
        {
            return {keyIndex(), valueIndex()};
        }

        constexpr int keyIndex() const
        {
            return -2;
        }

        constexpr int valueIndex() const
        {
            return -1;
        }

    private:
        static constexpr int BadIndex = 0;

        iterator(lua_State* l, int tableIndex) :
            m_lua(l),
            m_tableIndex(tableIndex)
        {
        }

        iterator& takeNext();

        lua_State* m_lua = nullptr;
        int m_tableIndex = BadIndex;

        friend class TableEnumerator;

        // actually comparing only with end
        friend bool operator==(const iterator& i1, const iterator& i2)
        {
            return i1.m_tableIndex == i2.m_tableIndex;
        }

        friend bool operator!=(const iterator& i1, const iterator& i2)
        {
            return i1.m_tableIndex != i2.m_tableIndex;
        }
    };

    TableEnumerator(lua_State* l, int tableIndex);

    TableEnumerator(const UpValuesEnumerator&) = delete;

    iterator begin() const;

    iterator end() const
    {
        return {};
    }

    lua_State* const m_lua;
    const int m_tableIndex;
};

// MY_LUATOOLKIT_EXPORT
// Result<> loadBuffer(lua_State* l, std::string_view buffer, const char* chunkName);
/**
    @brief load chunk from stream and keep it as anonymous function on stack.
*/
MY_LUATOOLKIT_EXPORT Result<> load(lua_State* l, io::IStream& stream, const char* chunkName);

/**
    Load chunk from stream and execute its function, keep execution results on stack.
*/
MY_LUATOOLKIT_EXPORT Result<> execute(lua_State* l, io::IStream& stream, int retCount, const char* chunkName);

/**
    Load chunk from buffer and execute its function, keep execution results on stack.
 */

MY_LUATOOLKIT_EXPORT Result<> execute(lua_State* l, std::span<const std::byte> program, int retCount, const char* chunkName);

/**
    Load chunk from string and execute its function, keep execution results on stack.
 */
inline Result<> execute(lua_State* l, std::string_view program, int retCount, const char* chunkName)
{
    const std::span<const std::byte> buffer{reinterpret_cast<const std::byte*>(program.data()), program.size()};
    return execute(l, buffer, retCount, chunkName);
}

inline int getAbsoluteStackPos(lua_State* l, int index)
{
    MY_DEBUG_ASSERT(index != 0);
    if (index > 0 || index <= LUA_REGISTRYINDEX)
    {
        return index;
    }

    const int top = lua_gettop(l);
    const int pos = top + (index + 1);
    MY_DEBUG_ASSERT(pos > 0);
    return pos;
}

inline std::string_view ToStringView(lua_State* l, int idx)
{
    MY_DEBUG_ASSERT(lua_type(l, idx) == LUA_TSTRING);
    size_t len = 0;
    const char* const str = lua_tolstring(l, idx, &len);
    return {str, len};
}

template <typename Call>
int SafeCall(lua_State* l, Call call)
{
    static_assert(std::is_invocable_r_v<Result<int>, Call, lua_State*>);

    {
        Result<int> res = call(l);
        if (res)
        {
            return *res;
        }

        const std::string_view errMessage = res.getError()->what();
        lua_pushlstring(l, errMessage.data(), errMessage.size());
    }
    lua_error(l);
    return 0;
}

inline Result<> checkErr(lua_State* l, int ret) noexcept
{
    if (ret == 0)
    {
        return ResultSuccess;
    }

    ErrorPtr error = MakeError(lua::ToStringView(l, -1));
    lua_pop(l, 1);
    return Result<>{std::move(error)};
}

}  // namespace my::lua

#define Lua_CheckErr(l, r) CheckResult(my::lua::checkErr(l, (r)))

#define Lua_ReturnError(l, Text, ...)                                \
    do                                                               \
    {                                                                \
        const std::string message = std::format(Text, #__VA_ARGS__); \
        lua_pushnil(l);                                              \
        lua_pushlstring(l, message.data(), message.size());          \
        return 2;                                                    \
    }                                                                \
    while (false)

#define Lua_CheckResult(l, r)                                       \
    do                                                              \
    {                                                               \
        if (!r)                                                     \
        {                                                           \
            const std::string message = r.getError()->getMessage(); \
            lua_pushnil(l);                                         \
            lua_pushlstring(l, message.data(), message.size());     \
            return 2;                                               \
        }                                                           \
    }                                                               \
    while (false)

#define guard_lstack(luaState)                                \
    const ::my::lua::StackGuard ANONYMOUS_VAR(_luaStackGuard) \
    {                                                         \
        luaState                                              \
    }

#if MY_DEBUG_ASSERT_ENABLED
    #define assert_lstack_unchanged(luaState)                                                 \
        const ::my::lua::StackNotChangesAssertionGuard ANONYMOUS_VAR(_luaStackUnchangedGuard) \
        {                                                                                     \
            luaState                                                                          \
        }
#else
    #define assert_lstack_unchanged(luaState)
#endif