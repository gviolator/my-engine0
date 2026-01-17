// #my_engine_source_file
#include "lua_debug_utils.h"
#include "lua_debuggee_impl.h"
#include "lua_debugger_state.h"
#include "lua_toolkit/lua_utils.h"
#include "my/io/file_system.h"
#include "my/serialization/runtime_value_builder.h"
#include "my/service/service_provider.h"

namespace my::lua {

namespace {

void CollectSourceMaps(io::FileSystem& fs, const io::FsPath& path, std::vector<debug::SourceMapPtr>& sourceMaps)
{
    for (const io::FsEntry& entry : io::DirectoryIterator{&fs, path})
    {
        if (entry.kind == io::FsEntryKind::File)
        {
            mylog_info("Look ({})", entry.path.getCStr());
            if (entry.path.getExtension() == ".map")
            {
                mylog_info("Found source map ({})", entry.path.getCStr());
            }
        }
        else
        {
            CollectSourceMaps(fs, entry.path, sourceMaps);
        }
    }
}

}  // namespace

/**
 */
struct LuaDebuggeeImpl::LuaLock
{
    ILuaApplication& app;
    lua_State* const l;

    LuaLock(ILuaApplication& app_in) :
        app(app_in),
        l(app.lock())
    {
        MY_DEBUG_ASSERT(l);
    }

    ~LuaLock()
    {
        app.lock();
    }

    operator lua_State*() const
    {
        return l;
    }
};

/**
 */
SourceBp::SourceBp(unsigned bpId, unsigned sourceId, const dap::SourceBreakpoint& bp) noexcept
    :
    m_id(bpId),
    m_sourceId(sourceId),
    m_bp(bp)
{
}

unsigned SourceBp::GetId() const
{
    return m_id;
}
unsigned SourceBp::GetSourceId() const
{
    return m_sourceId;
}
const dap::SourceBreakpoint& SourceBp::GetBreakpoint() const
{
    return m_bp;
}

const dap::Source& SourceBp::GetSource(const LuaDebuggeeImpl& debuggee) const
{
    return debuggee.GetSource(m_sourceId);
}

/**
 */
FunctionBp::FunctionBp(unsigned bpId, const dap::FunctionBreakpoint& bp) noexcept :
    m_id(bpId),
    m_bp(bp)
{
}

unsigned FunctionBp::GetId() const
{
    return m_id;
}

const dap::FunctionBreakpoint& FunctionBp::GetBreakpoint() const
{
    return m_bp;
}

SourceEntry::SourceEntry(unsigned id, dap::Source source) noexcept :
    m_id(id),
    m_source(std::move(source))
{
}

unsigned SourceEntry::GetId() const
{
    return m_id;
}
const dap::Source& SourceEntry::GetSource() const
{
    return m_source;
}

/**
 */
LuaDebuggeeImpl::LuaDebuggeeImpl(LuaAppPtr&& app, dap::IDebugSession& session) :
    m_app(std::move(app)),
    m_session(session)
{
    MY_DEBUG_ASSERT(m_app);
}

VariableManager& LuaDebuggeeImpl::GetVariableManager()
{
    return (m_variableManager);
}

unsigned LuaDebuggeeImpl::GetNextStackFrameId()
{
    return ++m_nextFrameId;
}

async::ExecutorPtr LuaDebuggeeImpl::getExecutor()
{
    return m_app ? m_app->getExecutor() : nullptr;
}

async::Task<> LuaDebuggeeImpl::disconnect()
{
    const LuaLock l{*m_app};
    DebuggerState::ReleaseState(l);
    lua_sethook(l, std::exchange(m_prevLuaHook, nullptr), 0, 0);

    co_return;
}

// Capabilities initializeSession(const InitializeRequestArguments& initArgs){}

Result<> LuaDebuggeeImpl::configureLaunch(RuntimeValuePtr configuration)
{
    return kResultSuccess;
}

Result<> LuaDebuggeeImpl::configureAttach(RuntimeValuePtr configuration)
{
    MY_DEBUG_ASSERT(!m_debugConfig);

    CheckResult(my::runtimeValueApply(m_debugConfig.emplace(), configuration));

    if (m_debugConfig->outputDir.empty())
    {
        return MakeError("'outputDir' is required");
    }

    std::vector<debug::SourceMapPtr> sourceMaps;
    CollectSourceMaps(getServiceProvider().get<io::FileSystem>(), "/scripts/compiled", sourceMaps);

    return kResultSuccess;
}

Result<> LuaDebuggeeImpl::configurationDone()
{
    const LuaLock l{*m_app};

    guard_lstack(l);

    DebuggerState::InstallState(l, this, [](void* ptr) noexcept -> lua_Debug*
    {
        LuaDebuggeeImpl* const this_ = static_cast<LuaDebuggeeImpl*>(ptr);
        return nullptr;
    });

    m_prevLuaHook = lua_gethook(l);

    lua_sethook(l, [](lua_State* const l, lua_Debug* const ar) noexcept
    {
        LuaDebuggeeImpl* const this_ = static_cast<LuaDebuggeeImpl*>(DebuggerState::GetClientPtr(l));
        MY_DEBUG_ASSERT(this_);
        if (this_)
        {
            this_->DebugTrace(l, ar);
        }
    }, LUA_MASKRET | LUA_MASKCALL | LUA_MASKLINE, 0);

    return kResultSuccess;
}

void LuaDebuggeeImpl::requestPause()
{
}

Result<std::vector<dap::Breakpoint>> LuaDebuggeeImpl::setBreakpoints(const dap::SetBreakpointsArguments& arg)
{
#if 0
    unsigned int id = 1;

    for (const dap::SourceBreakpoint& inBp : arg.breakpoints)
    {
        auto& outBp = breakpoints.emplace_back();
        outBp.verified = true;
        outBp.id = id++;
        outBp.message = "Some message";
    }

    return Result{std::move(breakpoints)};
#endif
    // lock_(_mutex);

    /*if (!_debugConfiguration->projectRoot.empty() && _debugConfiguration->projectRoot.length() < arg.source.path.length())
    {
        arg.source.path = arg.source.path.substr(_debugConfiguration->projectRoot.length() + 1);
    }*/

    const unsigned sourceId = StoreSource(arg.source);

    {
        // This method called every time when active breakpoints set is changed. That means it will be called when breakpoint is disabled or removed.
        // Just reset all breakpoints for specified source.
        auto iter = std::remove_if(m_sourceBreakpoints.begin(), m_sourceBreakpoints.end(), [sourceId](const SourceBp& bp)
        {
            return bp.GetSourceId() == sourceId;
        });

        if (const size_t removedCount = std::distance(iter, m_sourceBreakpoints.end()); removedCount > 0)
        {
            m_sourceBreakpoints.resize(m_sourceBreakpoints.size() - removedCount);
        }
    }

    std::vector<dap::Breakpoint> breakpoints;
    breakpoints.reserve(arg.breakpoints.size());

    for (const dap::SourceBreakpoint& inBp : arg.breakpoints)
    {
        dap::Breakpoint& outBp = breakpoints.emplace_back();
        outBp.id = ++m_nextBpId;

        // TODO: implement br resolver
        outBp.verified = true;
        // bp.message = "Not implemented yet";
        // bp.source = std::move(bp.source);
        outBp.line = inBp.line;

        m_sourceBreakpoints.emplace_back(*outBp.id, sourceId, inBp);
    }

    return Result{std::move(breakpoints)};
}

Result<std::vector<dap::Breakpoint>> LuaDebuggeeImpl::setFunctionBreakpoints(const dap::SetFunctionBreakpointsArguments& arg)
{
    std::vector<dap::Breakpoint> breakpoints;
    breakpoints.resize(arg.breakpoints.size());

    m_functionBreakpoints.clear();

    for (const dap::FunctionBreakpoint& inBp : arg.breakpoints)
    {
        dap::Breakpoint& outBp = breakpoints.emplace_back();
        outBp.id = ++m_nextBpId;

        // TODO: implement br resolver
        outBp.verified = true;

        m_functionBreakpoints.emplace_back(*outBp.id, inBp);
    }
    return {};
}

Result<std::vector<dap::Thread>> LuaDebuggeeImpl::getThreads()
{
    return {};
}

Result<std::vector<dap::Variable>> LuaDebuggeeImpl::getVariables(const dap::VariablesArguments&)
{
    return {};
}

Result<dap::EvaluateResponseBody> LuaDebuggeeImpl::evaluate(const dap::EvaluateArguments&)
{
    return {};
}

void LuaDebuggeeImpl::DebugTrace(lua_State* const l, lua_Debug* const ar) noexcept
{
    MY_DEBUG_ASSERT(!m_currentAr);
    m_currentAr = ar;
    scope_on_leave
    {
        m_currentAr = nullptr;
    };

    std::optional<dap::StoppedEventBody> stoppedEvent;

    if (ar->event == LUA_HOOKLINE && ar->source && ar->currentline > 0)
    {
        if (m_isPauseRequested.exchange(false))
        {
            // m_isPauseRequested.store(false, std::memory_order_relaxed);
            stoppedEvent.emplace("pause", "Pause on client request");
        }
    }

    // no pause request, check for breakpoints
    if (!stoppedEvent)
    {
        stoppedEvent = CheckPauseOnBreakpoint(l, ar);
    }

    if (!stoppedEvent && m_stepBreakPredicate && m_stepBreakPredicate(l, ar))
    {
        m_stepBreakPredicate = nullptr;
        stoppedEvent.emplace("step", "Debug step");
    }

    if (!stoppedEvent)
    {
        return;
    }

    dap::ContinueExecutionMode continueMode = dap::ContinueExecutionMode::Continue;

    {
    }
}

std::optional<dap::StoppedEventBody> LuaDebuggeeImpl::CheckPauseOnBreakpoint(lua_State* l, lua_Debug* ar)
{
    const auto EvaluateCondition = [&](std::string_view expression) noexcept -> bool
    {
        if (expression.empty())
        {
            return true;
        }

        guard_lstack(l);

        auto result = [&]() -> Result<bool>
        {
            CheckResult(EvaluateExpression(expression, l, 0, EvaluateMode::Watch))

            if (lua_type(l, -1) != LUA_TBOOLEAN)
            {
                return MakeError("Expression must be evaluated to boolean");
            }

            return (lua_toboolean(l, -1) == 1);
        }();

        if (!result)
        {
            // this->Output(Format::format("Fail to evaluate bp's condition [{}]:({}), will break unconditionally", expression, result.GetError()->Message()));
            return true;
        }

        return *result;
    };

    // lock_(_mutex);

    if (ar->event == LUA_HOOKLINE && ar->source && ar->currentline > 0 && !m_sourceBreakpoints.empty())
    {
        lua_getinfo(l, "nSl", ar);

        auto bp = std::find_if(m_sourceBreakpoints.begin(), m_sourceBreakpoints.end(), [this, ar](const SourceBp& bp)
        {
            return dap::Source::EqualFast(bp.GetSource(*this), ar->source) && bp.GetBreakpoint().line == static_cast<unsigned>(ar->currentline);
        });

        if (bp != m_sourceBreakpoints.end())
        {
            return std::nullopt;
        }

        // Log point:
        if (!bp->GetBreakpoint().logMessage.empty())
        {
            // this->Output(bp->Bp().logMessage);
            return std::nullopt;
        }

        return dap::StoppedEventBody{"breakpoint", "Hit breakpoint"}.SetBreakpoint(bp->GetId());
    }

    if (ar->event == LUA_HOOKCALL && !m_functionBreakpoints.empty())
    {
        lua_getinfo(l, "nS", ar);
        if (!ar->name || !ar->what || strcmp(ar->what, "Lua") != 0)
        {
            return std::nullopt;
        }

        auto bp = std::find_if(m_functionBreakpoints.begin(), m_functionBreakpoints.end(), [ar](const FunctionBp& bp)
        {
            return bp.GetBreakpoint().name == ar->name;
        });

        if (bp == m_functionBreakpoints.end() || !EvaluateCondition(bp->GetBreakpoint().condition))
        {
            return std::nullopt;
        }

        return dap::StoppedEventBody{"function breakpoint", fmt::format("Hitting function ({})", bp->GetBreakpoint().name)}
            .SetBreakpoint(bp->GetId());
    }

    return std::nullopt;
}

unsigned LuaDebuggeeImpl::StoreSource(const dap::Source& source)
{
    auto existingSource = std::find_if(m_sources.begin(), m_sources.end(), [&source](const SourceEntry& entry)
    {
        return entry.GetSource() == source;
    });

    if (existingSource != m_sources.end())
    {
        return existingSource->GetId();
    }

    return m_sources.emplace_back(++m_nextSourceId, std::move(source)).GetId();
}

const dap::Source& LuaDebuggeeImpl::GetSource(unsigned sourceId) const
{
    auto source = std::find_if(m_sources.begin(), m_sources.end(), [&sourceId](const SourceEntry& entry)
    {
        return entry.GetId() == sourceId;
    });
    MY_DEBUG_FATAL(source != m_sources.end());

    return source->GetSource();
}

dap::StartDebugSessionResult createLuaDebugSession(LuaAppPtr luaApp, const dap::InitializeRequestArguments&, dap::IDebugSession& session)
{
    dap::Capabilities caps;

    caps.supportsConfigurationDoneRequest = true;
    caps.supportsFunctionBreakpoints = true;
    caps.supportsConditionalBreakpoints = true;

    caps.supportsStepBack = false;
    caps.supportsLogPoints = true;
    caps.supportsDataBreakpoints = false;

    std::unique_ptr<LuaDebuggeeImpl> debuggee = std::make_unique<LuaDebuggeeImpl>(std::move(luaApp), session);
    return {std::move(caps), std::move(debuggee)};
}

}  // namespace my::lua
