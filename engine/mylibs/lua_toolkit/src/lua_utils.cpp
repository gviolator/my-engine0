// #my_engine_source_file

#include "chunk_loader.h"
#include "lua_toolkit/lua_interop.h"
#include "lua_toolkit/lua_utils.h"
#include "my/io/memory_stream.h"
#include "my/memory//runtime_stack.h"
#include "my/utils/scope_guard.h"

namespace my::lua {
StackGuard::StackGuard(lua_State* l) :
    luaState(l),
    top(lua_gettop(luaState))
{
}

StackGuard::~StackGuard()
{
    const int currentTop = lua_gettop(luaState);
    if (currentTop == top)
    {
        return;
    }

    // it is possible to restore the stack only if it is larger than the current one.
    // Potentially, there may be situations where the logic (is the opposite) removes values from the stack;
    // in this case, consider the use of lua::StackGuard to be incorrect.
    if (top < currentTop)
    {
        lua_settop(luaState, top);
    }
    else
    {
        // LOG_WARN("The lua's stack size has decreased. Value popped from stack unexpectedly (the bug ?) or stack guard should not be used.");
        // if (IsRunningUnderDebugger()) {
        // 	BreakIntoDebugger();
        // }
    }
}


UpValuesEnumerator::iterator::iterator() = default;

UpValuesEnumerator::iterator::iterator(lua_State* l, int funcIndex, int upvalIndex) :
    m_lua(l),
    m_funcIndex(funcIndex),
    m_upvalIndex(upvalIndex)
{
    MY_DEBUG_ASSERT(m_lua);
    MY_DEBUG_ASSERT(m_upvalIndex > 0);

    if (m_name = lua_getupvalue(m_lua, m_funcIndex, m_upvalIndex); m_name == nullptr)
    {
        m_upvalIndex = -1;
    }
}

UpValuesEnumerator::iterator& UpValuesEnumerator::iterator::operator++()
{
    MY_DEBUG_ASSERT(m_lua);
    MY_DEBUG_ASSERT(m_name);
    MY_DEBUG_ASSERT(m_upvalIndex > 0);

    lua_pop(m_lua, 1);

    if (m_name = lua_getupvalue(m_lua, m_funcIndex, ++m_upvalIndex); m_name == nullptr)
    {
        m_upvalIndex = -1;
    }

    return *this;
}

std::string_view UpValuesEnumerator::iterator::operator*() const
{
    return this->Name();
}

std::string_view UpValuesEnumerator::iterator::Name() const
{
    MY_DEBUG_ASSERT(m_upvalIndex > 0 && m_name);
    return std::string_view{m_name};
}

int UpValuesEnumerator::iterator::Index() const
{
    MY_DEBUG_ASSERT(m_upvalIndex > 0);
    return m_upvalIndex;
}

bool operator==(const UpValuesEnumerator::iterator& iter1, const UpValuesEnumerator::iterator& iter2)
{
    return iter1.m_upvalIndex == iter2.m_upvalIndex;
}

bool operator!=(const UpValuesEnumerator::iterator& iter1, const UpValuesEnumerator::iterator& iter2)
{
    return iter1.m_upvalIndex != iter2.m_upvalIndex;
}

UpValuesEnumerator::UpValuesEnumerator(lua_State* l, int funcIndex) :
    m_lua(l),
    m_funcIndex(funcIndex)
{
}

UpValuesEnumerator::iterator UpValuesEnumerator::begin() const
{
    return iterator{m_lua, m_funcIndex, 1};
}

UpValuesEnumerator::iterator UpValuesEnumerator::end() const
{
    return iterator{};
}

// StackLocalsEnumerator
StackLocalsEnumerator::iterator::iterator() = default;

StackLocalsEnumerator::iterator::iterator(lua_State* l, lua_Debug* ar, int n) :
    m_lua(l),
    m_activationRec(ar),
    m_index(n)
{
    MY_DEBUG_ASSERT(m_lua);
    MY_DEBUG_ASSERT(m_activationRec);

    if (m_name = lua_getlocal(m_lua, m_activationRec, m_index); m_name == nullptr)
    {
        m_index = -1;
    }
}

StackLocalsEnumerator::iterator& StackLocalsEnumerator::iterator::operator++()
{
    MY_DEBUG_ASSERT(m_lua);
    MY_DEBUG_ASSERT(m_name);
    MY_DEBUG_ASSERT(m_index > 0);

    lua_pop(m_lua, 1);

    if (m_name = lua_getlocal(m_lua, m_activationRec, ++m_index); m_name == nullptr)
    {
        m_index = -1;
    }

    return *this;
}

std::string_view StackLocalsEnumerator::iterator::operator*() const
{
    return this->Name();
}

std::string_view StackLocalsEnumerator::iterator::Name() const
{
    MY_DEBUG_ASSERT(m_index > 0 && m_name);
    return std::string_view{m_name};
}

int StackLocalsEnumerator::iterator::Index() const
{
    MY_DEBUG_ASSERT(m_index > 0);
    return m_index;
}

bool operator==(const StackLocalsEnumerator::iterator& iter1, const StackLocalsEnumerator::iterator& iter2)
{
    return iter1.m_index == iter2.m_index;
}

bool operator!=(const StackLocalsEnumerator::iterator& iter1, const StackLocalsEnumerator::iterator& iter2)
{
    return iter1.m_index != iter2.m_index;
}

StackLocalsEnumerator::StackLocalsEnumerator(lua_State* l, lua_Debug* ar) :
    m_lua(l),
    m_activationRec(ar)
{
}

StackLocalsEnumerator::iterator StackLocalsEnumerator::begin() const
{
    return iterator{m_lua, m_activationRec, 1};
}

StackLocalsEnumerator::iterator StackLocalsEnumerator::end() const
{
    return iterator{};
}

// TableEnumerator
TableEnumerator::TableEnumerator(lua_State* l, int tableIndex) :
    m_lua(l),
    m_tableIndex(getAbsoluteStackPos(l, tableIndex))
{
}

TableEnumerator::iterator TableEnumerator::begin() const
{
    MY_DEBUG_ASSERT(lua_type(m_lua, m_tableIndex) == LUA_TTABLE);
    lua_pushnil(m_lua);
    return iterator{m_lua, m_tableIndex}.takeNext();
}

TableEnumerator::iterator& TableEnumerator::iterator::operator++()
{
    MY_DEBUG_ASSERT(m_lua && m_tableIndex != BadIndex);

    lua_pop(m_lua, 1);  // pop out last value
    return takeNext();
}

TableEnumerator::iterator& TableEnumerator::iterator::takeNext()
{
    if (lua_next(m_lua, m_tableIndex) == 0)
    {
        m_tableIndex = BadIndex;
        m_lua = nullptr;
    }

    return *this;
}

Result<> load(lua_State* l, io::IStream& stream, const char* chunkName)
{
    rtstack_scope;

    ChunkLoader loader{stream, GetRtStackAllocator(), 1024};
    Lua_CheckErr(l, lua_load(l, ChunkLoader::read, &loader, chunkName, "tb"));

    return kResultSuccess;
}

Result<> execute(lua_State* l, io::IStream& stream, int retCount, const char* chunkName)
{
    CheckResult(load(l, stream, chunkName));
    Lua_CheckErr(l, lua_pcall(l, 0, retCount, 0));

    return kResultSuccess;
}

Result<> execute(lua_State* l, std::span<const std::byte> program, int retCount, const char* chunkName)
{
    if (program.empty())
    {
        return MakeError("program is empty");
    }

    rtstack_scope;

    Ptr<io::IStream> stream = io::createReadonlyMemoryStream(program, GetRtStackAllocatorPtr());
    return execute(l, *stream, retCount, chunkName);
}

}  // namespace my::lua
