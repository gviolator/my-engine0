// #my_engine_source_file
#pragma once

#include "lua_toolkit/debug/lua_debuggee.h"
#include "lua_variable_manager.h"
#include "my/debug/dap/dap.h"
#include "my/debug/dap/debug_session.h"
#include "my/debug/source_map.h"
#include "my/utils/functor.h"

#include <atomic>
#include <optional>
#include <string>
#include <vector>

namespace my::lua {

class LuaDebuggeeImpl;

/**
 */
struct DebugConfig : public dap::DebugConfigurationBase
{
    MY_CLASS_BASE(dap::DebugConfigurationBase)
    MY_CLASS_FIELDS(
        CLASS_FIELD(outputDir),
        CLASS_FIELD(sourceDir),
        CLASS_FIELD(sourceMapRoot))

    std::string outputDir;
    std::string sourceDir;
    std::string sourceMapRoot;
};

/**
 */
class SourceBp
{
public:
    SourceBp() noexcept = default;
    SourceBp(unsigned bpId, unsigned sourceId, const dap::SourceBreakpoint&) noexcept;
    SourceBp(const SourceBp&) = default;
    SourceBp(SourceBp&&) = default;

    SourceBp& operator=(const SourceBp&) = default;
    SourceBp& operator=(SourceBp&&) = default;

    unsigned GetId() const;
    unsigned GetSourceId() const;
    const dap::SourceBreakpoint& GetBreakpoint() const;
    const dap::Source& GetSource(const LuaDebuggeeImpl&) const;

private:
    unsigned m_id = 0;
    unsigned m_sourceId = 0;
    dap::SourceBreakpoint m_bp;
};

/**
 */
class FunctionBp
{
public:
    FunctionBp() noexcept = default;
    FunctionBp(unsigned, const dap::FunctionBreakpoint&) noexcept;
    FunctionBp(const FunctionBp&) = default;
    FunctionBp(FunctionBp&&) = default;

    FunctionBp& operator=(const FunctionBp&) = default;
    FunctionBp& operator=(FunctionBp&&) = default;
    unsigned GetId() const;
    const dap::FunctionBreakpoint& GetBreakpoint() const;

private:
    unsigned m_id = 0;
    dap::FunctionBreakpoint m_bp;
};

/**
 */
class SourceEntry
{
public:
    SourceEntry() noexcept = default;
    SourceEntry(unsigned id, dap::Source) noexcept;
    unsigned GetId() const;
    const dap::Source& GetSource() const;

private:
    unsigned m_id;
    dap::Source m_source;
};

/**
 */
class LuaDebuggeeImpl final : public dap::IDebuggeeControl
{
public:
    LuaDebuggeeImpl(LuaAppPtr&& app, dap::IDebugSession& session);

    VariableManager& GetVariableManager();

    unsigned GetNextStackFrameId();

private:
    async::ExecutorPtr getExecutor() override;

    async::Task<> disconnect() override;

    // virtual Capabilities initializeSession(const InitializeRequestArguments& initArgs) override;

    Result<> configureLaunch(RuntimeValuePtr configuration) override;

    Result<> configureAttach(RuntimeValuePtr configuration) override;

    Result<> configurationDone() override;

    void requestPause() override;

    Result<std::vector<dap::Breakpoint>> setBreakpoints(const dap::SetBreakpointsArguments&) override;

    Result<std::vector<dap::Breakpoint>> setFunctionBreakpoints(const dap::SetFunctionBreakpointsArguments&) override;

    Result<std::vector<dap::Thread>> getThreads() override;

    Result<std::vector<dap::Variable>> getVariables(const dap::VariablesArguments&) override;

    Result<dap::EvaluateResponseBody> evaluate(const dap::EvaluateArguments&) override;

    void DebugTrace(lua_State*, lua_Debug*) noexcept;
    std::optional<dap::StoppedEventBody> CheckPauseOnBreakpoint(lua_State* l, lua_Debug* ar);
    unsigned StoreSource(const dap::Source& source);
    const dap::Source& GetSource(unsigned sourceId) const;

    struct LuaLock;
    using StepBreakPredicate = Functor<bool(lua_State*, lua_Debug*) noexcept>;

    LuaAppPtr m_app;
    dap::IDebugSession& m_session;
    VariableManager m_variableManager;

    lua_Hook m_prevLuaHook = nullptr;
    lua_Debug* m_currentAr = nullptr;

    unsigned m_nextBpId = 0;
    unsigned m_nextSourceId = 0;
    unsigned m_nextFrameId = 0;

    std::optional<DebugConfig> m_debugConfig;
    std::vector<SourceBp> m_sourceBreakpoints;
    std::vector<FunctionBp> m_functionBreakpoints;
    std::vector<SourceEntry> m_sources;

    std::atomic<bool> m_isPauseRequested = false;
    StepBreakPredicate m_stepBreakPredicate;

    friend class SourceBp;
};

}  // namespace my::lua
