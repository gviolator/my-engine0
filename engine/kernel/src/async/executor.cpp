// #my_engine_source_file
#include "my/async/executor.h"

#include "my/utils/scope_guard.h"

namespace my::async
{
    namespace
    {
        static Executor::WeakPtr s_defaultExecutor;
        static thread_local Executor::WeakPtr s_thisThreadExecutor;
        static thread_local Executor::InvokeGuard* s_thisThreadInvokeGuard = nullptr;

        inline Executor* getThisThreadInvokedExecutor()
        {
            return s_thisThreadInvokeGuard ? &s_thisThreadInvokeGuard->executor : nullptr;
        }

    }  // namespace

    Executor::InvokeGuard::InvokeGuard(Executor& exec) :
        executor(exec),
        threadId(std::this_thread::get_id()),
        prev(s_thisThreadInvokeGuard)
    {
        s_thisThreadInvokeGuard = this;
    }

    Executor::InvokeGuard::~InvokeGuard()
    {
        MY_DEBUG_CHECK(threadId == std::this_thread::get_id());
        MY_DEBUG_CHECK(s_thisThreadInvokeGuard == this);

        s_thisThreadInvokeGuard = s_thisThreadInvokeGuard->prev;
    }

    Executor::Invocation::~Invocation() = default;

    Executor::Invocation::Invocation(Callback cb, void* cbData1, void* cbData2) :
        m_callback(cb),
        m_callbackData1(cbData1),
        m_callbackData2(cbData2)
    {
    }

    Executor::Invocation::Invocation(Invocation&& other) :
        m_callback(other.m_callback),
        m_callbackData1(other.m_callbackData1),
        m_callbackData2(other.m_callbackData2)
    {
        other.reset();
    }

    Executor::Invocation& Executor::Invocation::operator=(Invocation&& other)
    {
        m_callback = other.m_callback;
        m_callbackData1 = other.m_callbackData1;
        m_callbackData2 = other.m_callbackData2;

        other.reset();
        return *this;
    }

    Executor::Invocation::operator bool() const
    {
        return m_callback != nullptr;
    }

    void Executor::Invocation::operator()()
    {
        MY_DEBUG_CHECK(m_callback);
        scope_on_leave
        {
            reset();
        };

        if (!m_callback) [[unlikely]]
        {
            return;
        }

        m_callback(m_callbackData1, m_callbackData2);
    }

    void Executor::Invocation::reset()
    {
        m_callback = nullptr;
        m_callbackData1 = nullptr;
        m_callbackData2 = nullptr;
    }

    Executor::Invocation Executor::Invocation::fromCoroutine(std::coroutine_handle<> coroutine)
    {
        MY_DEBUG_CHECK(coroutine);
        if (!coroutine)
        {
            return {};
        }

        return Invocation{[](void* coroAddress, void*) noexcept
        {
            MY_DEBUG_CHECK(coroAddress);
            std::coroutine_handle<> coroutine = std::coroutine_handle<>::from_address(coroAddress);
            coroutine();
        },
                          coroutine.address(), nullptr};
    }

    Executor::Ptr Executor::getDefault()
    {
        return s_defaultExecutor.acquire();
    }

    Executor::Ptr Executor::getInvoked()
    {
        return getThisThreadInvokedExecutor();
    }

    Executor::Ptr Executor::getThisThreadExecutor()
    {
        return s_thisThreadExecutor.acquire();
    }

    Executor::Ptr Executor::getCurrent()
    {
        if (auto* const invokedExecutor = getThisThreadInvokedExecutor())
        {
            return invokedExecutor;
        }

        auto threadExecutor = s_thisThreadExecutor.acquire();
        return threadExecutor ? threadExecutor : s_defaultExecutor.acquire();
    }

    void Executor::setDefault(Executor::Ptr executor)
    {
        s_defaultExecutor = std::move(executor);
    }

    void Executor::setThisThreadExecutor(Executor::Ptr executor)
    {
        s_thisThreadExecutor = std::move(executor);
    }

    // void Executor::setExecutorName(Executor::Ptr executor, std::string_view name);

    // Executor::Ptr Executor::findByName(std::string_view name);

    void Executor::finalize(Executor::Ptr&& executor)
    {
        using namespace std::chrono;

        MY_DEBUG_CHECK(executor);
        if (!executor)
        {
            return;
        }

        auto checkFinalizeTooLong = [time = system_clock::now(), notified = false]() mutable
        {
            constexpr milliseconds ShutdownTimeout{5s};

            if (system_clock::now() - time >= ShutdownTimeout && !notified)
            {
                notified = true;
                // core::dumpActiveTasks();
            }
        };

        do
        {
            executor->waitAnyActivity();
            if (executor->getRefsCount() == 1)  // the only local reference
            {
                break;
            }

            checkFinalizeTooLong();
        } while (true);
    }

    void Executor::execute(Invocation invocation) noexcept
    {
        scheduleInvocation(std::move(invocation));
    }

    void Executor::execute(std::coroutine_handle<> coroutine) noexcept
    {
        MY_DEBUG_CHECK(coroutine);
        scheduleInvocation(Invocation::fromCoroutine(std::move(coroutine)));
    }

    void Executor::execute(Callback callback, void* data1, void* data2) noexcept
    {
        scheduleInvocation(Invocation{callback, data1, data2});
    }

    void Executor::invoke([[maybe_unused]] Executor& executor, Invocation invocation) noexcept
    {
        MY_DEBUG_CHECK(getThisThreadInvokedExecutor() != nullptr, "Executor must be set prior invoke. Use Executor::InvokeGuard.");
        MY_DEBUG_CHECK(getThisThreadInvokedExecutor() == &executor, "Invalid executor.");
        MY_DEBUG_CHECK(invocation);

        invocation();
    }

    void Executor::invoke([[maybe_unused]] Executor& executor, std::span<Invocation> invocations) noexcept
    {
        MY_DEBUG_CHECK(getThisThreadInvokedExecutor() != nullptr, "Executor must be set prior invoke. Use Executor::InvokeGuard.");
        MY_DEBUG_CHECK(getThisThreadInvokedExecutor() == &executor, "Invalid executor.");

        for (auto& invocation : invocations)
        {
            invocation();
        }
    }

}  // namespace my::async
