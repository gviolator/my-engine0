#pragma once
#include "my/app/application_api.h"
#include "my/async/work_queue.h"
#include "my/memory/singleton_memop.h"
#include "my/module/module_manager.h"
#include "my/rtti/rtti_impl.h"
#include "my/runtime/internal/kernel_runtime.h"


namespace my
{
    class ApplicationImpl : public Application
    {
        MY_RTTI_CLASS(my::ApplicationImpl, Application)

    public:
        ApplicationImpl(void (*shutdownCoreServicesCallback)());

        ~ApplicationImpl();

        AppState getState() const override;
        bool isMainThread() const override;
        Result<> startupOnCurrentThread() override;
        bool step() override;
        void stop() override;

    private:
        Result<> startupServices();
        async::Task<> shutdownServicesAndRuntime();

        // At this moment core services (including global PropertyContainer must exists).
        KernelRuntimePtr m_kernelRuntime = createKernelRuntime();
        ModuleManagerPtr m_moduleManager = createModuleManager();
        std::atomic<AppState> m_appState = AppState::NotStarted;
        std::thread::id m_appThreadId;
        WorkQueuePtr m_appWorkQueue;

        async::Task<> m_shutdownTask;
        Functor<bool()> m_kernelRuntimeShutdown;
        void (*m_shutdownCoreServicesCallback)();
    };
}  // namespace my
