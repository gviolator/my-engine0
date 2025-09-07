// #my_engine_source_file
#include "application_impl.h"

#include "my/app/application.h"
#include "my/module/module_manager.h"
#include "my/service/internal/service_provider_initialization.h"


namespace my
{
    ApplicationImpl::ApplicationImpl(void (*shutdownCoreServicesCallback)()) :
        m_shutdownCoreServicesCallback(shutdownCoreServicesCallback)
    {
        MY_FATAL(!applicationExists());
        app_detail::setApplicationInstance(*this);
    }

    ApplicationImpl::~ApplicationImpl()
    {
        MY_ASSERT(app_detail::resetApplicationInstance() == this);

        m_shutdownCoreServicesCallback();
    }

    AppState ApplicationImpl::getState() const
    {
        return m_appState.load(std::memory_order_relaxed);
    }

    bool ApplicationImpl::isMainThread() const
    {
        MY_DEBUG_ASSERT(m_appThreadId != std::thread::id{});
        return m_appThreadId == std::this_thread::get_id();
    }

    Result<> ApplicationImpl::startupOnCurrentThread()
    {
        MY_FATAL(m_appThreadId == std::thread::id{}, "Application already has started");
        MY_FATAL(m_appState == AppState::NotStarted);

        scope_on_leave
        {
            if (m_appState == AppState::Initializing)
            {
                m_appState = AppState::Active;
            }
        };

        m_appState = AppState::Initializing;
        m_appThreadId = std::this_thread::get_id();
        m_appWorkQueue = createWorkQueue();
        m_appWorkQueue->setName("Application work queue");
        async::Executor::setThisThreadExecutor(m_appWorkQueue);

        return startupServices();
    }

    Result<> ApplicationImpl::startupServices()
    {
        const auto waitTaskAndCheckResult = [&](async::Task<> task) -> Result<>
        {
            while (!task.isReady())
            {
                m_appWorkQueue->poll();
            }

            Result<> initResult = !task.isRejected() ? ResultSuccess : task.getError();
            if (!initResult)
            {
                m_appState = AppState::Invalid;
            }

            return initResult;
        };

        // TODO: handle invalid initialization: 
        // if service/module failed to init, then application must be reverted to initial state.
        CheckResult(m_moduleManager->doModulesPhase(ModuleManager::ModulesPhase::Init));

        ServiceProvider& serviceProvider = getServiceProvider();
        auto& serviceInit = serviceProvider.as<core_detail::IServiceProviderInitialization&>();

        CheckResult(waitTaskAndCheckResult(serviceInit.preInitServices()));
        CheckResult(waitTaskAndCheckResult(serviceInit.initServices()));

        CheckResult(m_moduleManager->doModulesPhase(ModuleManager::ModulesPhase::PostInit));

        return ResultSuccess;
    }

    async::Task<> ApplicationImpl::shutdownServicesAndRuntime()
    {
        [[maybe_unused]] const auto prevAppState = m_appState.exchange(AppState::RuntimeShutdownProcessed);
        MY_DEBUG_ASSERT(prevAppState == AppState::GameShutdownProcessed);

        async::Task<> servicesShutdown = getServiceProvider().as<core_detail::IServiceProviderInitialization&>().shutdownServices();
        m_kernelRuntimeShutdown = m_kernelRuntime->shutdown(false);  // keep kernel service till application destruction

        return servicesShutdown;
    }

    bool ApplicationImpl::step()
    {
        MY_FATAL(m_appThreadId != std::thread::id{}, "Application is not started");
        MY_ASSERT(m_appThreadId == std::this_thread::get_id(), "Invalid thread");

        const AppState lastAppState = m_appState.load(std::memory_order_relaxed);

        if (lastAppState == AppState::ShutdownCompleted)
        {
            return false;
        }

        m_appWorkQueue->poll();

        if (lastAppState == AppState::Active)
        {
        }
        else if (lastAppState == AppState::ShutdownRequested)
        {
            m_appState = AppState::GameShutdownProcessed;
        }
        else if (lastAppState == AppState::GameShutdownProcessed)
        {
            m_shutdownTask = shutdownServicesAndRuntime();
        }
        else if (lastAppState == AppState::RuntimeShutdownProcessed)
        {
            MY_DEBUG_FATAL(m_kernelRuntimeShutdown);
            MY_DEBUG_FATAL(m_shutdownTask);

            const bool runtimeShutdownCompleted = !m_kernelRuntimeShutdown();
            if (runtimeShutdownCompleted)
            {
                MY_DEBUG_ASSERT(m_shutdownTask.isReady());

                m_appState = AppState::ShutdownCompleted;

                // 1. clear all global services
                setDefaultServiceProvider(nullptr);

                // 2. unload modules
                m_moduleManager->doModulesPhase(ModuleManager::ModulesPhase::Shutdown).ignore();
                m_moduleManager.reset();
            }
        }

        return m_appState.load(std::memory_order_relaxed) != AppState::ShutdownCompleted;
    }

    void ApplicationImpl::stop()
    {
        AppState expectedState = AppState::Active;
        if (!m_appState.compare_exchange_strong(expectedState, AppState::ShutdownRequested))
        {
            mylog_debug("Application state not active, possible application already stopped ?");
        }
    }

}  // namespace my
