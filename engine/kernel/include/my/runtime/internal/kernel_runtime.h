// #my_engine_source_file
#pragma once

#include "my/async/executor.h"
#include "my/kernel/kernel_config.h"
#include "my/utils/functor.h"

#include <memory>
#include <thread>

namespace my {
enum class RuntimePollMode
{
    Default,
    NoWait
};

struct IKernelRuntime
{
    virtual ~IKernelRuntime() = default;

    virtual void bindToCurrentThread() = 0;

    virtual std::thread::id getRuntimeThreadId() const = 0;

    virtual async::ExecutorPtr getRuntimeExecutor() = 0;

    virtual bool poll(RuntimePollMode mode) = 0;

    /**
        @p resetKernelServices
            if true then shutdown will reset kernel services
            in other case reset will be performed within KernelRuntime's destructor.
    */
    // virtual Functor<bool()> shutdown(bool resetKernelServices = true) = 0;
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
