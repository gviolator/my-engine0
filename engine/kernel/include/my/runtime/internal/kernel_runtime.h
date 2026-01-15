// #my_engine_source_file
#pragma once

#include "my/async/executor.h"
#include "my/kernel/kernel_config.h"
#include "my/utils/functor.h"

#include <memory>
#include <thread>

namespace my {

/**
 * Polling mode for IKernelRuntime::poll()
 */
enum class RuntimePollMode
{
    Default,
    NoWait
};

/**
 */
enum class RuntimeState
{
    NotInitialized,          // Initial state, runtime is not bound to any thread yet.
    Operable,                // Runtime is bound to a thread, initialized and can be used.
    ShutdownProcessed,       // Shutdown() was called, runtime is shutting down, but still has references.
    ShutdownNeedCompletion,  // Runtime is ready to be completed, all references are released.
    ShutdownCompleted        // Runtime is completely shut down.
};

struct IKernelRuntime
{
    virtual ~IKernelRuntime() = default;

    virtual RuntimeState getState() const = 0;

    virtual void bindToCurrentThread() = 0;

    virtual std::thread::id getRuntimeThreadId() const = 0;

    virtual async::ExecutorPtr getRuntimeExecutor() = 0;

    virtual bool poll(RuntimePollMode mode) = 0;

    virtual void shutdown() = 0;

    inline bool isRuntimeThread() const
    {
        return getRuntimeThreadId() == std::this_thread::get_id();
    }
};

using KernelRuntimePtr = std::unique_ptr<IKernelRuntime>;

MY_KERNEL_EXPORT KernelRuntimePtr createKernelRuntime();

MY_KERNEL_EXPORT bool kernelRuntimeExists();

MY_KERNEL_EXPORT IKernelRuntime& getKernelRuntime();

}  // namespace my
