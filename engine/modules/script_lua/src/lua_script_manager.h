// #my_engine_source_file
#pragma once
#include "my/containers/intrusive_list.h"
#include "my/debug/dap/debugger_host.h"
#include "my/io/fs_path.h"
#include "my/rtti/rtti_impl.h"
#include "my/script/script_manager.h"
#include "my/service/service.h"
#include "my/runtime/async_disposable.h"

namespace my::script {

class LuaScriptManager final : public virtual IServiceInitialization,
                               public virtual IServiceShutdown,

                               public IScriptManager,
                               public dap::IDebuggerHost,
                               public virtual IRefCounted
{
    MY_REFCOUNTED_CLASS(my::script::LuaScriptManager,
                        IServiceInitialization,
                        IServiceShutdown,
                        IScriptManager,
                        dap::IDebuggerHost,
                        IRefCounted)

public:
    async::Task<> initService() override;
    async::Task<> shutdownService() override;

    Result<io::FsPath> ResolvePath(io::FsPath path) override;

    void RegisterRealm(class LuaRealm&);
    void UnregisterRealm(LuaRealm&);

private:
    void AddSearchPath(io::FsPath path) override;
    void AddFileExtension(std::string_view ext) override;
    void EnableDebug(bool enableDebug) override;

    std::vector<DebugLocation> GetDebugLocations() override;
    async::Task<dap::StartDebugSessionResult> StartDebugSession(std::string locationId, const dap::InitializeRequestArguments& initArgs, dap::IDebugSession& session) override;

    std::vector<io::FsPath> m_searchPaths;
    std::string m_scriptFileExtension = ".lua";
    IntrusiveList<LuaRealm> m_realms;
    Ptr<IAsyncDisposable> m_debuggerHost;
};
}  // namespace my::script
