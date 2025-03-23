// #my_engine_source_header
#pragma once

#include <chrono>
#include <memory>

#include "my/async/cpp_coroutine.h"
#include "my/async/executor.h"
#include "my/diag/error.h"
#include "my/rtti/rtti_object.h"
#include "my/kernel/kernel_config.h"

namespace my::async
{
   /**
     */
    struct MY_ABSTRACT_TYPE ITimerManager : virtual IRttiObject
    {
        MY_INTERFACE(my::async::ITimerManager, IRttiObject);

        using InvokeAfterHandle = uint64_t;
        using InvokeAfterCallback = void (*)(void*) noexcept;
        using ExecuteAfterCallback = void (*)(Error::Ptr, void*) noexcept;


        MY_KERNEL_EXPORT
        static void setInstance(std::unique_ptr<ITimerManager>);

        MY_KERNEL_EXPORT
        static ITimerManager& getInstance();

        MY_KERNEL_EXPORT
        static bool hasInstance();

        MY_KERNEL_EXPORT
        static std::unique_ptr<ITimerManager> createDefault();


        static inline void setDefaultInstance()
        {
            setInstance(createDefault());
        }

        static inline void releaseInstance()
        {
            setInstance(nullptr);
        }

        virtual void executeAfter(std::chrono::milliseconds timeout, async::Executor::Ptr, ExecuteAfterCallback callback, void* callbackData) = 0;

        virtual InvokeAfterHandle invokeAfter(std::chrono::milliseconds timeout, InvokeAfterCallback, void*) = 0;

        virtual void cancelInvokeAfter(InvokeAfterHandle) = 0;
    };

    /**
        @brief
        Implement await with explicit timeout:
        co_await 100ms;
    */
    inline void executeAfter(std::chrono::milliseconds timeout, async::Executor::Ptr executor, ITimerManager::ExecuteAfterCallback callback, void* callbackData)
    {
        ITimerManager::getInstance().executeAfter(timeout, std::move(executor), callback, callbackData);
    }

    /**

    */
    inline auto invokeAfter(std::chrono::milliseconds timeout, ITimerManager::InvokeAfterCallback callback, void* data)
    {
        return ITimerManager::getInstance().invokeAfter(timeout, callback, data);
    }

    /**
     */
    inline void cancelInvokeAfter(ITimerManager::InvokeAfterHandle handle)
    {
        return ITimerManager::getInstance().cancelInvokeAfter(handle);
    }

}  // namespace my::async
