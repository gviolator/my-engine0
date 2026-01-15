// #my_engine_source_file
#pragma once

//#include "lua_variable_manager.h"
#include "my/debug/dap/dap.h"
#include "my/debug/dap/stack_trace_provider.h"

//#include "lua-toolkit/debug/luadebug.h"
#include "lua_toolkit/lua_header.h"
#include "lua_variable_manager.h"

#include "my/rtti/rtti_impl.h"

#include <string_view>
#include <optional>
#include <memory>
#include <vector>

namespace my::lua {

class LuaDebuggeeImpl;

/*
 */
class StackTraceProvider final : public dap::IStackTraceProvider
{
    MY_TYPEID(my::lua::StackTraceProvider)
    MY_CLASS_BASE(dap::IStackTraceProvider)
    //MY_RTTI_CLASS(my::lua::StackTraceProvider, dap::IStackTraceProvider)

public:
    StackTraceProvider(lua_State*, lua_Debug*, LuaDebuggeeImpl&);

    VariableManager& GetVariableManager() const;

    std::optional<int> GetLevelOfFrame(unsigned frameId) const;

    dap::StackTraceResponseBody GetStackTrace(const dap::StackTraceArguments&) override;

    std::vector<dap::Scope> GetScopes(unsigned stackFrameId) override;

    Result<dap::EvaluateResponseBody> EvaluateWatch(const dap::EvaluateArguments&) override;

private:
    class StackFrameEntry
    {
    public:
        StackFrameEntry(StackTraceProvider& provider, unsigned frameId, int level);

        unsigned Id() const;

        int Level() const;

        const dap::StackFrame& GetFrameInfo() const;

        std::vector<dap::Scope> GetScopes(StackTraceProvider& provider);

    private:
        const unsigned _id;
        const int _level;
        dap::StackFrame _frameInfo;
        std::vector<dap::Scope> _scopes;
        VariableRegistration _localsVariable;
        VariableRegistration _closureVariable;
    };

    class StackScopeVariable;
    class StackLocalsVariable;
    class StackUpValuesVariable;
    class GlobalScopeVariable;

    std::string_view GetSourceRoot() const;

    dap::Scope GetGlobalsScope();

    StackFrameEntry* FindStackFrameEntry(unsigned frameId);

    const StackFrameEntry* FindStackFrameEntry(unsigned frameId) const;

    lua_State* const _lua;
    lua_Debug* const _ar;
    LuaDebuggeeImpl& m_debuggee;
    std::list<StackFrameEntry> _stackFrames;
    std::optional<dap::Scope> _globalsScope;
    VariableRegistration _globalsScopeVariableRef;
    VariableRegistration _watchVariable;
};

}  // namespace Lua::Debug
