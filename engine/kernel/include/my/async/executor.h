// #my_engine_source_header
#pragma once
#include <span>

#include <thread>
#include <type_traits>

#include "my/async/cpp_coroutine.h"
#include "my/diag/check.h"
#include "my/rtti/ptr.h"
#include "my/rtti/rtti_object.h"
#include "my/rtti/weak_ptr.h"
#include "my/kernel/kernel_config.h"
#include "my/utils/functor.h"
#include "my/utils/preprocessor.h"

namespace my::async
{

    /**
     */
    class MY_ABSTRACT_TYPE Executor : public virtual IRefCounted
    {
        MY_INTERFACE(my::async::Executor, IRefCounted)

    public:
        using Ptr = my::Ptr<Executor>;
        using WeakPtr = my::WeakPtr<Executor>;

        using Callback = void (*)(void* data1, void* data2) noexcept;

        struct MY_KERNEL_EXPORT InvokeGuard
        {
            InvokeGuard(Executor& exec);
            InvokeGuard(const InvokeGuard&) = delete;
            InvokeGuard(InvokeGuard&&) = delete;

            ~InvokeGuard();

            Executor& executor;
            const std::thread::id threadId;
            InvokeGuard* const prev = nullptr;
        };

        class MY_KERNEL_EXPORT Invocation
        {
        public:
            static Invocation fromCoroutine(std::coroutine_handle<> coroutine);

            Invocation() = default;

            Invocation(Callback, void* data1, void* data2);

            Invocation(Invocation&&);

            Invocation(const Invocation&) = delete;

            ~Invocation();

            Invocation& operator=(Invocation&&);

            Invocation& operator=(const Invocation&) = delete;

            explicit operator bool() const;

            void operator()();

        private:
            void reset();

            Callback m_callback = nullptr;
            void* m_callbackData1 = nullptr;
            void* m_callbackData2 = nullptr;
        };

        /**
         */
        MY_KERNEL_EXPORT static Executor::Ptr getDefault();

        /**
         */
        MY_KERNEL_EXPORT static Executor::Ptr getInvoked();

        /**
        */
        MY_KERNEL_EXPORT static Executor::Ptr getThisThreadExecutor();

        /**
         */
        MY_KERNEL_EXPORT static Executor::Ptr getCurrent();

        /**
         */
        MY_KERNEL_EXPORT static void setDefault(Executor::Ptr);

        /**
         */
        MY_KERNEL_EXPORT static void setThisThreadExecutor(Executor::Ptr executor);

        MY_KERNEL_EXPORT static void setExecutorName(Executor::Ptr executor, std::string_view name);

        /**
         */
        MY_KERNEL_EXPORT static Executor::Ptr findByName(std::string_view name);

        MY_KERNEL_EXPORT static void finalize(Executor::Ptr&& executor);

        /**
         */
        MY_KERNEL_EXPORT void execute(Invocation invocation) noexcept;

        MY_KERNEL_EXPORT void execute(std::coroutine_handle<>) noexcept;

        MY_KERNEL_EXPORT void execute(Callback, void* data1, void* data2 = nullptr) noexcept;

        virtual void waitAnyActivity() noexcept = 0;

    protected:
        MY_KERNEL_EXPORT static void invoke(Executor&, Invocation) noexcept;

        MY_KERNEL_EXPORT static void invoke(Executor&, std::span<Invocation> invocations) noexcept;

        virtual void scheduleInvocation(Invocation) noexcept = 0;
    };

    /*
     *
     */
    struct ExecutorAwaiter
    {
        Executor::Ptr executor;

        ExecutorAwaiter(Executor::Ptr exec) :
            executor(std::move(exec))
        {
            MY_DEBUG_CHECK(executor, "Executor must be specified");
        }

        constexpr bool await_ready() const noexcept
        {
            return false;
        }

        void await_suspend(std::coroutine_handle<> continuation) const
        {
            if(executor)
            {
                executor->execute(std::move(continuation));
            }
        }

        constexpr void await_resume() const noexcept
        {
        }
    };

    inline ExecutorAwaiter operator co_await(Executor::Ptr executor)
    {
        return {std::move(executor)};
    }

    inline ExecutorAwaiter operator co_await(Executor::WeakPtr executorWeakRef)
    {
        auto executor = executorWeakRef.acquire();
        MY_DEBUG_CHECK(executor, "Executor instance expired");

        return {std::move(executor)};
    }

}  // namespace my::async

#define ASYNC_SWITCH_EXECUTOR(executorExpression)                         \
    {                                                                     \
        ::my::async::Executor::Ptr executorVar = executorExpression;     \
        MY_DEBUG_CHECK(executorVar);                                            \
                                                                          \
        if(my::async::Executor::getCurrent().get() != executorVar.get()) \
        {                                                                 \
            co_await executorVar;                                         \
        }                                                                 \
    }\
