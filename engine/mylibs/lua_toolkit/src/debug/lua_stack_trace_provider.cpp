#include "lua_debuggee_impl.h"
#include "lua_debug_utils.h"
#include "lua_stack_trace_provider.h"
#include "lua_toolkit/lua_utils.h"
#include "my/diag/logging.h"

#include <fmt/format.h>

#include <filesystem>

// #include <exception>
namespace fs = std::filesystem;

namespace my::lua {

/**
 *
 */
class StackTraceProvider::StackScopeVariable
{
    MY_TYPEID(my::lua::StackTraceProvider::StackScopeVariable)

public:
    lua_Debug* GetAr() const
    {
        return _provider._ar;
    }

    unsigned FrameId() const
    {
        return _frameId;
    }

protected:
    StackScopeVariable(StackTraceProvider& provider, unsigned frameId) :
        _provider(provider),
        _frameId(frameId)
    {
    }

    lua_Debug* ActivateStackFrame()
    {
        StackFrameEntry* const frame = _provider.FindStackFrameEntry(_frameId);
        MY_DEBUG_ASSERT(frame);
        if (!frame)
        {
            return nullptr;
        }

        lua_Debug* const ar = _provider._ar;

        return lua_getstack(_provider._lua, frame->Level(), ar) != 0 ? ar : nullptr;
    }

private:
    StackTraceProvider& _provider;
    const unsigned _frameId;
};

/**
 *
 */
class StackTraceProvider::StackLocalsVariable final : public CompoundVariable,
                                                      public StackScopeVariable
{
    MY_RTTI_CLASS(my::lua::CompoundVariable, CompoundVariable)

public:
    StackLocalsVariable(StackTraceProvider& provider, unsigned frameId) :
        CompoundVariable(provider._lua),
        StackScopeVariable(provider, frameId)
    {
    }

    bool PushChildOnStack(ChildVariableKey key) override
    {
        lua_Debug* const ar = ActivateStackFrame();
        MY_DEBUG_ASSERT(ar);
        if (!ar)
        {
            return false;
        }

        lua_State* const l = this->GetLua();
        const char* const name = lua_getlocal(l, ar, static_cast<int>(key));
        return name != nullptr;
    }

    std::vector<VariableKeyValue> GetChildVariables(VariableManager& manager) override
    {
        lua_Debug* const ar = ActivateStackFrame();
        MY_DEBUG_ASSERT(ar);
        if (!ar)
        {
            return {};
        }

        std::vector<VariableKeyValue> variables;
        lua_State* const l = this->GetLua();

        const StackLocalsEnumerator enumerator{l, ar};
        for (auto local = enumerator.begin(), end = enumerator.end(); local != end; ++local)
        {
            if (local.Name().find("(*") != 0)
            {
                variables.emplace_back(manager.CreateVariableFromLuaStack(l, -1, *this, ChildVariableKey(local.Index()), local.Name()));
            }
        }

        return variables;
    }
};

/**
 *
 */
class StackTraceProvider::StackUpValuesVariable final : public ClosureVariableBase,
                                                        public StackScopeVariable
{
    MY_RTTI_CLASS(my::lua::StackTraceProvider::StackUpValuesVariable)

public:
    StackUpValuesVariable(StackTraceProvider& provider, unsigned frameId) :
        ClosureVariableBase(provider._lua),
        StackScopeVariable(provider, frameId)
    {
    }

private:
    bool PushSelfOnStack() override
    {
        lua_Debug* const ar = ActivateStackFrame();
        MY_DEBUG_ASSERT(ar);
        if (!ar)
        {
            return false;
        }

        lua_State* const l = this->GetLua();
        return lua_getinfo(l, "fu", ar) != 0;
    }
};

/**
 *
 */
class StackTraceProvider::GlobalScopeVariable final : public TableVariableBase
{
    MY_RTTI_CLASS(my::lua::StackTraceProvider::GlobalScopeVariable::TableVariableBase)
public:
    GlobalScopeVariable(StackTraceProvider& provider) :
        //TableVariableBase(provider._lua, LUA_GLOBALSINDEX)
        TableVariableBase(provider._lua, -1)
    {
    }

private:
    bool PushSelfOnStack() override
    {
        //lua_pushvalue(this->GetLua(), LUA_GLOBALSINDEX);
        lua_pushvalue(this->GetLua(), -1);
        return true;
    }
};

/* -------------------------------------------------------------------------- */

StackTraceProvider::StackFrameEntry::StackFrameEntry(StackTraceProvider& provider, unsigned frameId, int level) :
    _id(frameId),
    _level(level),
    _frameInfo(_id, {})
{
    lua_Debug* const ar = provider._ar;

    lua_getinfo(provider._lua, "nSl", ar);

    if (strcmp(ar->what, "main") == 0)
    {
        _frameInfo.name = "[chunk]";
    }
    else if (strcmp(ar->what, "Lua") == 0)
    {
        _frameInfo.name = ar->name ? ar->name : "[UNNAMED]";
    }
    else if (strcmp(ar->what, "C") == 0)
    {
        _frameInfo.name = ar->name ? fmt::format("{} (C/C++)", ar->name) : "[UNNAMED] (C/C++)";
    }

    if (ar->source)
    {
        if (*ar->source == '=')
        {
            _frameInfo.line = 0;
        }
        else
        {
            _frameInfo.line = ar->currentline;

            fs::path path{ar->source};

            if (!path.is_absolute() && !provider.GetSourceRoot().empty())
            {
                path = fs::path{std::string(provider.GetSourceRoot())} / path;
            }

            if (fs::exists(path))
            {
                path = fs::canonical(path);
            }
            else
            {
                mylog_debug("Fail to resolve path: {}", ar->source);
            }

            _frameInfo.source.emplace(path.string());

            [[maybe_unused]] dap::Source& source = *_frameInfo.source;
        }
    }
}

unsigned StackTraceProvider::StackFrameEntry::Id() const
{
    return _id;
}

int StackTraceProvider::StackFrameEntry::Level() const
{
    return _level;
}

const dap::StackFrame& StackTraceProvider::StackFrameEntry::GetFrameInfo() const
{
    return _frameInfo;
}

std::vector<dap::Scope> StackTraceProvider::StackFrameEntry::GetScopes(StackTraceProvider& provider)
{
    if (!_scopes.empty())
    {
        return _scopes;
    }

    lua_Debug* const ar = provider._ar;
    lua_State* const l = provider._lua;

    if (lua_getstack(l, _level, ar) == 0)
    {
        mylog_warn("Fail to activate stack frame");
        return {};
    }

    lua_getinfo(l, "nSlfu", ar);

    //_scopes.emplace_back(provider.GetGlobalsScope());

    {
        this->_localsVariable = provider.GetVariableManager().RegisterVariableRoot(std::make_unique<StackLocalsVariable>(provider, _id));
        dap::Scope& scope = _scopes.emplace_back("Locals", this->_localsVariable.VariableId());
        scope.line = ar->linedefined;
        scope.endLine = ar->lastlinedefined;
        scope.presentationHint = "locals";
    }

    if (ar->nups > 0)
    {
        this->_closureVariable = provider.GetVariableManager().RegisterVariableRoot(std::make_unique<StackUpValuesVariable>(provider, _id));
        dap::Scope& scope = _scopes.emplace_back("Closure", this->_closureVariable.VariableId());
        scope.presentationHint = "arguments";
    }

    return _scopes;
}

/* -------------------------------------------------------------------------- */
StackTraceProvider::StackTraceProvider(lua_State* luaState, lua_Debug* ar, LuaDebuggeeImpl& debuggee) :
    _lua(luaState),
    _ar(ar),
    m_debuggee(debuggee)
{
}

VariableManager& StackTraceProvider::GetVariableManager() const
{
    return m_debuggee.GetVariableManager();
}

std::optional<int> StackTraceProvider::GetLevelOfFrame(unsigned frameId) const
{
    const auto frame = FindStackFrameEntry(frameId);
    return frame ? std::optional<int>{frame->Level()} : std::nullopt;
}

dap::StackTraceResponseBody StackTraceProvider::GetStackTrace(const dap::StackTraceArguments& args)
{
    if (_stackFrames.empty())
    {
        for (int level = 0; lua_getstack(_lua, level, _ar) != 0; ++level)
        {
            _stackFrames.emplace_back(*this, m_debuggee.GetNextStackFrameId(), level);
        }
    }

    dap::StackTraceResponseBody response;
    response.stackFrames.reserve(_stackFrames.size());
    std::transform(_stackFrames.begin(), _stackFrames.end(), std::back_inserter(response.stackFrames), [](const StackFrameEntry& f) -> dap::StackFrame
    {
        return f.GetFrameInfo();
    });

    return response;
}

std::vector<dap::Scope> StackTraceProvider::GetScopes(unsigned frameId)
{
    auto frame = FindStackFrameEntry(frameId);
    return frame ? frame->GetScopes(*this) : std::vector<dap::Scope>{};
}

Result<dap::EvaluateResponseBody> StackTraceProvider::EvaluateWatch(const dap::EvaluateArguments& args)
{
    dap::EvaluateResponseBody response;

    StackFrameEntry* const frame = args.frameId ? FindStackFrameEntry(*args.frameId) : nullptr;
    if (!frame)
    {
        return MakeError("Invalid stack frame id");
    }

    if (!_watchVariable)
    {
        _watchVariable = GetVariableManager().RegisterVariableRoot(RefStorageVariable::Create(_lua));
    }

    auto& refStorage = _watchVariable->as<RefStorageVariable&>();

    CheckResult(EvaluateExpression(args.expression, _lua, frame->Level(), EvaluateMode::Watch))
    auto [key, variable] = GetVariableManager().CreateVariableFromLuaStack(_lua, -1, refStorage, nullptr);
    response = dap::EvaluateResponseBody::FromVariable(std::move(variable));

    return response;
}

StackTraceProvider::StackFrameEntry* StackTraceProvider::FindStackFrameEntry(unsigned frameId)
{
    auto frame = std::find_if(_stackFrames.begin(), _stackFrames.end(), [frameId](const StackFrameEntry& entry)
    {
        return entry.Id() == frameId;
    });
    return frame != _stackFrames.end() ? &(*frame) : nullptr;
}

const StackTraceProvider::StackFrameEntry* StackTraceProvider::FindStackFrameEntry(unsigned frameId) const
{
    auto frame = std::find_if(_stackFrames.begin(), _stackFrames.end(), [frameId](const StackFrameEntry& entry)
    {
        return entry.Id() == frameId;
    });
    return frame != _stackFrames.end() ? &(*frame) : nullptr;
}

std::string_view StackTraceProvider::GetSourceRoot() const
{
    // MY_DEBUG_ASSERT(_sessionController._debugConfiguration);
    // return _sessionController._debugConfiguration->projectRoot;
    return {};
}

dap::Scope StackTraceProvider::GetGlobalsScope()
{
    if (!_globalsScope)
    {
        auto variable = std::make_unique<GlobalScopeVariable>(*this);

        _globalsScope.emplace("GLOBALSINDEX");
        _globalsScope->expensive = true;
        _globalsScope->indexedVariables = variable->IndexedFieldsCount();
        _globalsScope->namedVariables = variable->NamedFieldsCount();

        _globalsScopeVariableRef = GetVariableManager().RegisterVariableRoot(std::move(variable));
        _globalsScope->variablesReference = _globalsScopeVariableRef;
    }

    return *_globalsScope;
}

}  // namespace my::lua
