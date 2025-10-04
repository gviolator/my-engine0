// #my_engine_source_file
#include "kernel_runtime_impl.h"
#include "my/async/async_timer.h"
#include "my/async/thread_pool_executor.h"
#include "my/diag/logging.h"
#include "my/memory/fixed_size_block_allocator.h"
#include "my/runtime/disposable.h"
#include "my/runtime/internal/runtime_component.h"
#include "my/runtime/internal/runtime_object_registry.h"
#include "runtime/runtime_executor.h"
#include "runtime/uv_handle.h"
#include "runtime/uv_utils.h"

using namespace my::async;
using namespace my::my_literals;

namespace my {

namespace {

constexpr Byte RuntimeMemorySize = 2_Mb;
static KernelRuntimeImpl* s_kernelRuntime = nullptr;

}  // namespace

namespace async {

MY_KERNEL_EXPORT bool hasAliveTasksWithCapturedExecutor();
MY_KERNEL_EXPORT void dumpAliveTasks();

}  // namespace async

KernelRuntimeImpl::KernelRuntimeImpl()
{
    MY_DEBUG_FATAL(s_kernelRuntime == nullptr);
    s_kernelRuntime = this;

    setDefaultRuntimeObjectRegistryInstance();
    ITimerManager::setDefaultInstance();
    m_defaultExecutor = createThreadPoolExecutor();
    Executor::setDefault(m_defaultExecutor);

    m_runtimeMemory = createHostVirtualMemory(RuntimeMemorySize, true);
}

KernelRuntimeImpl::~KernelRuntimeImpl()
{
    scope_on_leave
    {
        MY_DEBUG_ASSERT(s_kernelRuntime == this);
        s_kernelRuntime = nullptr;
    };

    MY_DEBUG_ASSERT(m_state == State::ShutdownCompleted || m_state == State::ShutdownNeedCompletion, "RuntimeState::shutdown() is not completely processed");
    if (m_state == State::ShutdownNeedCompletion)
    {
        completeShutdown();
    }
}

void KernelRuntimeImpl::bindToCurrentThread()
{
    MY_DEBUG_ASSERT(m_threadId == std::thread::id{});
    m_threadId = std::this_thread::get_id();

    UV_VERIFY(uv_loop_init(&m_uv));
    const size_t uvHandleBlockSize = getUvHandleMaxSize();
    m_uvHandleAllocator = createFixedSizeBlockAllocator(m_runtimeMemory, uvHandleBlockSize, false);
    m_internalHandles = decltype(m_internalHandles){m_uvHandleAllocator->getMemoryResource()};

    m_runtimeExecutor = rtti::createInstance<RuntimeThreadExecutor>();
    RuntimeObjectRegistration{m_runtimeExecutor}.setAutoRemove();
}

std::thread::id KernelRuntimeImpl::getRuntimeThreadId() const
{
    MY_DEBUG_ASSERT(m_threadId != std::thread::id{}, "Runtime not binded to thread yet");
    return m_threadId;
}

async::ExecutorPtr KernelRuntimeImpl::getRuntimeExecutor()
{
    MY_DEBUG_ASSERT(m_threadId != std::thread::id{} && m_runtimeExecutor);
    return m_runtimeExecutor;
}

bool KernelRuntimeImpl::poll(RuntimePollMode mode)
{
    MY_DEBUG_ASSERT(isRuntimeThread());
    if (!(m_state == State::Operable || m_state == State::ShutdownProcessed))
    {
        return false;
    }

    const uv_run_mode runMode =
        (m_state == State::Operable && mode == RuntimePollMode::Default) ? UV_RUN_DEFAULT : UV_RUN_NOWAIT;


    [[maybe_unused]] const bool hasReferences = uv_run(&m_uv, runMode) != 0;
    if (m_state == State::ShutdownProcessed)
    {
        const bool steelHasRuntimeReferences = shutdownStep(true);
        if (!steelHasRuntimeReferences)
        {
        }
    }

    return true;
}

#if 0
Functor<bool()> KernelRuntimeImpl::shutdown(bool doCompleteShutdown)
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
#endif
void KernelRuntimeImpl::shutdown()
{
    if (m_state != State::Operable)
    {
        mylog_warn("shutdown() called while runtime is not operable");
        return;
    }

    m_state = State::ShutdownProcessed;

    getRuntimeObjectRegistry().visitObjects<IDisposable>([](std::span<IRttiObject*> objects)
    {
        for (IRttiObject* const object : objects)
        {
            object->as<IDisposable&>().dispose();
        }
    });

    m_runtimeExecutor->execute([](void* selfPtr, void*) noexcept
    {
        KernelRuntimeImpl& self = *static_cast<KernelRuntimeImpl*>(selfPtr);
        self.m_shutdownStartTime = std::chrono::system_clock::now();
        self.m_shutdownTooLongWarningShowed = false;
        uv_stop(self.uv());
    }, this, nullptr);
}

bool KernelRuntimeImpl::shutdownStep(bool doCompleteShutdown)
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
    size_t externalUvHandlesCount = 0;

    // Checking registered runtime objects
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

    // Checking uv external references.
    // (anyway in most cases uv objects existence must correlate with runtime objects)
    auto walkState = std::tuple{this, &externalUvHandlesCount};
    uv_walk(&m_uv, [](uv_handle_t* handle, void* arg) noexcept
    {
        auto& [self, handleCounter] = *static_cast<decltype(walkState)*>(arg);
        const bool isInternalHandle = std::find(self->m_internalHandles.begin(), self->m_internalHandles.end(), handle) != self->m_internalHandles.end();
        if (!isInternalHandle)
        {
            ++handleCounter;
        }
    }, &walkState);


    bool canCompleteShutdown = !(hasPendingWorks || hasReferencedExecutors || hasReferencedNonExecutors || externalUvHandlesCount > 0);
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

void KernelRuntimeImpl::completeShutdown()
{
    MY_DEBUG_ASSERT(isRuntimeThread());
    MY_DEBUG_ASSERT(m_state == State::ShutdownNeedCompletion);
    scope_on_leave
    {
        m_state = State::ShutdownCompleted;
    };

    m_runtimeExecutor.reset();

    while (uv_loop_alive(&m_uv) != 0)
    {
        if (uv_run(&m_uv, UV_RUN_NOWAIT) == 0)
        {
            break;
        }
    }

    [[maybe_unused]] const int res = uv_loop_close(&m_uv);


    Executor::setDefault(nullptr);
    m_defaultExecutor.reset();

    ITimerManager::releaseInstance();
    resetRuntimeObjectRegistryInstance();
}

uv_loop_t* KernelRuntimeImpl::uv()
{
    return &m_uv;
}

IAllocator& KernelRuntimeImpl::getUvHandleAllocator() const
{
    return *m_uvHandleAllocator;
}

void KernelRuntimeImpl::setHandleAsInternal(const uv_handle_t* handle)
{
    MY_DEBUG_ASSERT(m_threadId == std::this_thread::get_id());
    m_internalHandles.push_back(handle);
}

KernelRuntimePtr createKernelRuntime()
{
    auto runtime = std::make_unique<KernelRuntimeImpl>();
    return runtime;
}

KernelRuntimeImpl& getKernelRuntimeImpl()
{
    MY_DEBUG_FATAL(s_kernelRuntime);
    return *s_kernelRuntime;
}

IKernelRuntime& getKernelRuntime()
{
    MY_DEBUG_FATAL(s_kernelRuntime);
    return *s_kernelRuntime;
}

}  // namespace my
