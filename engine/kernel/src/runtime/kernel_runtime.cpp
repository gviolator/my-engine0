// #my_engine_source_file
#include "my/runtime/internal/kernel_runtime.h"

#include "my/async/async_timer.h"
#include "my/async/thread_pool_executor.h"
#include "my/diag/logging.h"
#include "my/memory/singleton_memop.h"
#include "my/runtime/disposable.h"
#include "my/runtime/internal/runtime_component.h"
#include "my/runtime/internal/runtime_object_registry.h"

using namespace my::async;

namespace my::async
{
    MY_KERNEL_EXPORT bool hasAliveTasksWithCapturedExecutor();
    MY_KERNEL_EXPORT void dumpAliveTasks();
}  // namespace my::async

namespace my
{
    class KernelRuntimeImpl final : public KernelRuntime
    {
        MY_DECLARE_SINGLETON_MEMOP(KernelRuntimeImpl)
    public:
        KernelRuntimeImpl()
        {
            setDefaultRuntimeObjectRegistryInstance();
            ITimerManager::setDefaultInstance();
            m_defaultAsyncExecutor = createThreadPoolExecutor();
            Executor::setDefault(m_defaultAsyncExecutor);
        }

        ~KernelRuntimeImpl()
        {
            MY_DEBUG_ASSERT(m_state == State::ShutdownCompleted || m_state == State::ShutdownNeedCompletion, "RuntimeState::shutdown() is not completely processed");
            if (m_state == State::ShutdownNeedCompletion)
            {
                completeShutdown();
            }
        }

        Functor<bool()> shutdown(bool doCompleteShutdown) override
        {
            if (m_state != State::Operable)
            {
                mylog_warn("KernelRuntime::shutdown() called multiple times");
                return []
                {
                    return false;
                };
            }

            m_state = State::ShutdownProcessed;

            getRuntimeObjectRegistry().visitObjects<IDisposable>([](std::span<IRttiObject*> objects)
            {
                for (IRttiObject* const object : objects)
                {
                    object->as<IDisposable&>().dispose();
                }
            });

            m_shutdownStartTime = std::chrono::system_clock::now();
            m_shutdownTooLongWarningShowed = false;

            return [this, doCompleteShutdown]
            {
                return shutdownStep(doCompleteShutdown);
            };
        }

        void completeShutdown()
        {
            MY_DEBUG_ASSERT(m_state == State::ShutdownNeedCompletion);
            scope_on_leave
            {
                m_state = State::ShutdownCompleted;
            };

            Executor::setDefault(nullptr);
            m_defaultAsyncExecutor.reset();

            ITimerManager::releaseInstance();
            resetRuntimeObjectRegistryInstance();
        }

    private:
        enum class State
        {
            Operable,
            ShutdownProcessed,
            ShutdownNeedCompletion,
            ShutdownCompleted
        };

        bool shutdownStep(bool doCompleteShutdown)
        {
            namespace chrono = std::chrono;
            using namespace std::chrono_literals;

            if (m_state != State::ShutdownProcessed)
            {
                return false;
            }

            constexpr auto ShutdownTooLongTimeout = 3s;

            bool hasPendingWorks = false;
            bool hasReferencedExecutors = false;
            bool hasReferencedNonExecutors = false;

            getRuntimeObjectRegistry().visitObjects<IRuntimeComponent>([&](std::span<IRttiObject*> objects)
            {
                for (IRttiObject* const object : objects)
                {
                    auto& runtimeComponent = object->as<IRuntimeComponent&>();
                    if (runtimeComponent.hasWorks())
                    {
                        hasPendingWorks = true;
                    }
                    else if (IRefCounted* const refCounted = object->as<IRefCounted*>(); refCounted)
                    {
                        constexpr size_t ExecutorExpectedRefs = 2;
                        constexpr size_t NonExecutorExpectedRefs = 1;

                        const bool isExecutor = refCounted->is<async::Executor>();
                        const size_t expectedRefsCount = isExecutor ? ExecutorExpectedRefs : NonExecutorExpectedRefs;
                        const size_t currentRefsCount = refCounted->getRefsCount();

                        MY_DEBUG_ASSERT(currentRefsCount >= expectedRefsCount);
                        if (currentRefsCount > expectedRefsCount)
                        {
                            if (isExecutor)
                            {
                                hasReferencedExecutors = true;
                            }
                            else
                            {
                                hasReferencedNonExecutors = true;
                            }
                        }
                    }
                }
            });

            bool canCompleteShutdown = !(hasPendingWorks || hasReferencedExecutors || hasReferencedNonExecutors);
            if (!canCompleteShutdown && (!hasPendingWorks && !hasReferencedNonExecutors))
            {
                canCompleteShutdown = !async::hasAliveTasksWithCapturedExecutor();
                if (canCompleteShutdown)
                {
                    mylog_warn("The application will be forcefully completed, but there is still references to the executor. This could potentially be a source of problems.");
                }
            }

            if (canCompleteShutdown)
            {
                m_state = State::ShutdownNeedCompletion;
                if (doCompleteShutdown)
                {
                    completeShutdown();
                }
            }
            else if (chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - m_shutdownStartTime) > ShutdownTooLongTimeout)
            {
                if (!m_shutdownTooLongWarningShowed)
                {
                    m_shutdownTooLongWarningShowed = true;
                    mylog_warn("It appears that the application completion is blocked: either there is an unfinished task or some executor is still referenced");
                    async::dumpAliveTasks();
                }
            }

            return !canCompleteShutdown;
        }

        ExecutorPtr m_defaultAsyncExecutor;
        State m_state = State::Operable;
        std::chrono::system_clock::time_point m_shutdownStartTime;
        bool m_shutdownTooLongWarningShowed = false;
    };

    KernelRuntimePtr createKernelRuntime()
    {
        return std::make_unique<KernelRuntimeImpl>();
    }
}  // namespace my
