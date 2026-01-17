// #my_engine_source_file
#include "lua_realm.h"
#include "lua_script_manager.h"
#include "lua_toolkit/debug/lua_debuggee.h"
#include "my/app/property_container.h"
#include "my/io/file_system.h"
#include "my/service/service_provider.h"
#include "my/app/application_api.h"

namespace my::script {

namespace {

struct ScriptsConfig
{
    struct DebugInfo
    {
        bool enabled = false;
        std::string host;

        MY_CLASS_FIELDS(
            CLASS_FIELD(enabled),
            CLASS_FIELD(host))
    };

    DebugInfo debug;

#pragma region ClassInfo
    MY_CLASS_FIELDS(
        CLASS_FIELD(debug))
#pragma endregion
};




class LuaApp final : public lua::ILuaApplication
{
public:
    LuaApp(LuaRealm& realm): m_realm(realm)
    {}

private:
    lua_State* lock() override
    {
        return m_realm.GetLua();
    }

    void unlock() override
    {
    }

    async::ExecutorPtr getExecutor() override
    {
        return getApplication().getAppExecutor();
    }

    LuaRealm& m_realm;
};

}  // namespace

async::Task<> LuaScriptManager::initService()
{
    const ScriptsConfig config = getServiceProvider().get<PropertyContainer>().getValue<ScriptsConfig>("/scripts").value_or({});
    if (config.debug.enabled && !config.debug.host.empty())
    {
        network::Address address = *network::AddressFromString(config.debug.host);
        m_debuggerHost = co_await dap::RunDebuggerHost(address, Ptr{this});
    }

    co_return;
}

async::Task<> LuaScriptManager::shutdownService()
{
    if (m_debuggerHost)
    {
        co_await m_debuggerHost->disposeAsync();
    }

    co_return;
}

Result<io::FsPath> LuaScriptManager::ResolvePath(io::FsPath filePath)
{
    io::FileSystem& fs = getServiceProvider().get<io::FileSystem>();
    io::FsPath scriptFilePath;
    if (filePath.isAbsolute())
    {
        scriptFilePath = filePath;
    }
    else
    {
        for (const auto& scriptsRoot : m_searchPaths)
        {
            io::FsPath modulePath = (scriptsRoot / filePath);
            modulePath = modulePath + m_scriptFileExtension;

            if (fs.exists(modulePath, io::FsEntryKind::File))
            {
                scriptFilePath = std::move(modulePath);
                break;
            }
        }
    }

    if (scriptFilePath.isEmpty())
    {
        return MakeError("Script file path not resolved:({})", filePath.getString());
    }

    if (!fs.exists(scriptFilePath, io::FsEntryKind::File))
    {
        return MakeError("Path is not regular file:({})", filePath.getString());
    }

    return scriptFilePath;
}

// Result<io::FilePtr> LuaScriptManager::openScriptFile(const io::FsPath& filePath) const
// {

//     io::FilePtr file = fs.openFile(scriptFilePath, io::AccessMode::Read, io::OpenFileMode::OpenExisting);
//     if (!file)
//     {
//         return MakeError("Fail to open script file:({})", scriptFilePath.getString());
//     }
//     return file;
// }

// RealmPtr LuaScriptManager::createRealm()
// {
//     return nullptr;
// }

void LuaScriptManager::AddSearchPath(io::FsPath path)
{
    m_searchPaths.emplace_back(std::move(path));
}

void LuaScriptManager::AddFileExtension(std::string_view ext)
{
    m_scriptFileExtension = ext;
}

void LuaScriptManager::EnableDebug([[maybe_unused]] bool enableDebug)
{
}

std::vector<dap::IDebuggerHost::DebugLocation> LuaScriptManager::GetDebugLocations()
{
    return {
        {.id = "1", .description = "Default"}
    };
}

async::Task<dap::StartDebugSessionResult> LuaScriptManager::StartDebugSession(std::string locationId, const dap::InitializeRequestArguments& initArgs, dap::IDebugSession& session)
{
    MY_DEBUG_FATAL(!m_realms.empty());
    LuaRealm& realm = m_realms.front();
    co_return lua::createLuaDebugSession(std::make_unique<LuaApp>(realm), initArgs, session);
}

void LuaScriptManager::RegisterRealm(LuaRealm& realm)
{
    m_realms.push_back(realm);
}

void LuaScriptManager::UnregisterRealm(LuaRealm& realm)
{
    m_realms.remove(realm);
}

}  // namespace my::script
