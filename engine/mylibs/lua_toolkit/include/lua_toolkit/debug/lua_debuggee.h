// #my_engine_source_file
#pragma once

#include "lua_toolkit/lua_header.h"
#include "my/async/executor.h"
#include "my/debug/dap/dap.h"
#include "my/diag/assert.h"
// #include "my/debug/dap/debugee_control.h"
#include "my/debug/dap/debugger_host.h"

#include <memory>

namespace my::lua {

// Ptr<dap::IDebuggeeControl>

struct MY_ABSTRACT_TYPE ILuaApplication
{
    virtual ~ILuaApplication() = default;

    virtual lua_State* lock() = 0;
    virtual void unlock() = 0;
    virtual async::ExecutorPtr getExecutor() = 0;
};

using LuaAppPtr = std::unique_ptr<ILuaApplication>;

dap::StartDebugSessionResult createLuaDebugSession(LuaAppPtr luaApp, const dap::InitializeRequestArguments& initArgs, dap::IDebugSession& session);

}  // namespace my::lua
