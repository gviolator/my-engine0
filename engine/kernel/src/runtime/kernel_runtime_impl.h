// #my_engine_source_file
#pragma once
#include <uv.h>

#include "my/memory/host_memory.h"
#include "my/memory/allocator.h"
#include "my/memory/singleton_memop.h"
#include "my/runtime/internal/kernel_runtime.h"


namespace my {

class KernelRuntimeImpl final : public IKernelRuntime
{
    MY_DECLARE_SINGLETON_MEMOP(KernelRuntimeImpl)
public:
    KernelRuntimeImpl();
    ~KernelRuntimeImpl();

    void bindToCurrentThread() override;
    std::thread::id getRuntimeThreadId() const override;
    async::ExecutorPtr getRuntimeExecutor() override;
    bool poll(RuntimePollMode mode) override;
    void shutdown() override;

    uv_loop_t* uv();
    IAllocator& getUvHandleAllocator() const;
    void setHandleAsInternal(const uv_handle_t* handle);

private:
    enum class State
    {
        Operable,
        ShutdownProcessed,
        ShutdownNeedCompletion,
        ShutdownCompleted
    };

    bool shutdownStep(bool doCompleteShutdown);
    void completeShutdown();

    std::atomic<State> m_state = State::Operable;
    HostMemoryPtr m_runtimeMemory;
    AllocatorPtr m_uvHandleAllocator;
    async::ExecutorPtr m_defaultExecutor;
    async::ExecutorPtr m_runtimeExecutor;
    uv_loop_t m_uv;
    std::thread::id m_threadId = std::thread::id{};
    std::pmr::list<const uv_handle_t*> m_internalHandles;

    std::chrono::system_clock::time_point m_shutdownStartTime;
    bool m_shutdownTooLongWarningShowed = false;
};

KernelRuntimeImpl& getKernelRuntimeImpl();

}  // namespace my
