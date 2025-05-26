// #my_engine_source_file
#include "my/async/async_timer.h"
#include "my/async/task.h"
#include "my/diag/assert.h"
#include "my/diag/common_errors.h"
#include "my/memory/singleton_memop.h"
#include "my/runtime/disposable.h"
#include "my/runtime/internal/runtime_component.h"
#include "my/runtime/internal/runtime_object_registry.h"
#include "my/threading/lock_guard.h"
#include "my/threading/spin_lock.h"
#include "my/utils/scope_guard.h"
#include "my/containers/intrusive_list.h"

namespace my::async
{

    class Win32TimerManager final : public ITimerManager,
                                    public IRuntimeComponent,
                                    public IDisposable
    {
    public:
        MY_RTTI_CLASS(my::async::Win32TimerManager, IRuntimeComponent, IDisposable)
        MY_DECLARE_SINGLETON_MEMOP(Win32TimerManager)

        Win32TimerManager() :
            m_hTimerQueue(::CreateTimerQueue()),
            m_runtimeObjectRegistration(*this)
        {
        }

        ~Win32TimerManager()
        {
            MY_DEBUG_ASSERT(m_timerStateList.empty());

            ::DeleteTimerQueue(m_hTimerQueue);
        }

        void executeAfter(std::chrono::milliseconds timeout, async::Executor::Ptr executor, ExecuteAfterCallback callback, void* callbackData) override
        {
            executeAfterAsync(timeout, std::move(executor), callback, callbackData).detach();
        }

        Task<> executeAfterAsync(std::chrono::milliseconds timeout, async::Executor::Ptr executor, ExecuteAfterCallback callback, void* callbackData)
        {
            MY_DEBUG_ASSERT(callback);
            if (!callback)
            {
                co_return;
            }

            const auto stateId = getNextTimerId();
            TimerState state{*this, stateId, static_cast<DWORD>(timeout.count())};

            auto opResult = co_await state.getTask();
            Error::Ptr error;

            if (opResult == TimerOperationResult::Cancelled)
            {
                error = MakeErrorT(OperationCancelledError)();
            }
            else if (opResult == TimerOperationResult::ShutingDown)
            {
                error = MakeErrorT(OperationCancelledError)("Timers subsystem is disposed");
            }

            if (executor)
            {
                ASYNC_SWITCH_EXECUTOR(executor)
            }

            callback(std::move(error), callbackData);
        }

        InvokeAfterHandle invokeAfter(std::chrono::milliseconds delay, InvokeAfterCallback callback, void* data) override
        {
            const auto stateId = getNextTimerId();

            auto task = [](Win32TimerManager& self, DWORD delay, InvokeAfterHandle id, InvokeAfterCallback callback, void* data) -> async::Task<>
            {
                // state actually will be stored on heap, but its address must not be used as state_id
                //
                TimerState state{self, id, delay};

                // - TimerOperationResult::Success = callback must be called
                // - TimerOperationResult::Cancelled = explicitly cancelled ny client, callback must NOT be called
                // - TimerOperationResult::ShuttingDown = manager is in disposed state, callback must be called
                // - ...
                if (const auto opResult = co_await state.getTask(); opResult != TimerOperationResult::Cancelled)
                {
                    lock_(state.mutex);

                    if (!state.cancelled)
                    {
                        callback(data);
                    }
                }
            }(*this, static_cast<DWORD>(delay.count()), stateId, callback, data);

            task.detach();

            return stateId;
        }

        void cancelInvokeAfter(InvokeAfterHandle handle) override
        {
            if (handle == 0)
            {
                return;
            }

            lock_(m_mutex);

            auto state = std::find_if(m_timerStateList.begin(), m_timerStateList.end(), [handle](const TimerState& state)
            {
                return state.id == handle;
            });

            if (state != m_timerStateList.end())
            {
                state->cancel(TimerOperationResult::Cancelled);
            }
        }

        void dispose() override
        {
            if (m_isDisposed.exchange(true))
            {  // already disposed
                return;
            }

            lock_(m_mutex);
            for (auto& timerState : m_timerStateList)
            {
                timerState.cancel(TimerOperationResult::ShutingDown);
            }
        }

        bool hasWorks() override
        {
            lock_(m_mutex);
            return !m_timerStateList.empty();
        }

    private:
        enum class TimerOperationResult
        {
            Success,
            Cancelled,
            ShutingDown
        };

        struct TimerState : IntrusiveListNode<TimerState>
        {
            Win32TimerManager& manager;
            const InvokeAfterHandle id;
            HANDLE hTimer = nullptr;
            async::TaskSource<TimerOperationResult> promise;
            threading::SpinLock mutex;
            bool cancelled = false;

            TimerState(Win32TimerManager& m, InvokeAfterHandle stateId, DWORD timeMs) :
                manager(m),
                id(stateId)
            {
                using namespace my::async;

                lock_(manager.m_mutex);

                scope_on_leave
                {
                    manager.m_timerStateList.push_back(*this);
                };

                if (manager.m_isDisposed)
                {
                    cancelled = true;
                    promise.resolve(TimerOperationResult::Cancelled);
                    return;
                }

                const auto timerCallback = [](void* ptr, [[maybe_unused]]
                                                         BOOLEAN timerFaired)
                {
                    MY_DEBUG_ASSERT(timerFaired == TRUE);

                    auto promise = TaskSource<TimerOperationResult>::fromCoreTask(CoreTaskOwnership{reinterpret_cast<CoreTask*>(ptr)});
                    [[maybe_unused]]
                    const bool resolved = promise.resolve(TimerOperationResult::Success);
                };

                CoreTask* const coreTaskPtr = CoreTaskPtr{promise}.giveUp();

                [[maybe_unused]]
                const bool result = ::CreateTimerQueueTimer(&hTimer, manager.m_hTimerQueue, timerCallback, reinterpret_cast<void*>(coreTaskPtr), timeMs, 0, WT_EXECUTEDEFAULT);
                MY_DEBUG_ASSERT(result == TRUE);
            }

            ~TimerState()
            {
                {
                    lock_(manager.m_mutex);
                    manager.m_timerStateList.removeElement(*this);
                }

                if (hTimer)
                {
                    [[maybe_unused]] BOOL deletedOk = ::DeleteTimerQueueTimer(manager.m_hTimerQueue, hTimer, INVALID_HANDLE_VALUE);
                    MY_DEBUG_ASSERT(deletedOk == TRUE);
                }
            }

            async::Task<TimerOperationResult> getTask()
            {
                return promise.getTask();
            }

            void cancel(TimerOperationResult result)
            {
                {
                    lock_(mutex);
                    cancelled = true;
                }

                promise.resolve(result);
                if (auto timerHandle = std::exchange(hTimer, nullptr))
                {
                    [[maybe_unused]] BOOL deletedOk = ::DeleteTimerQueueTimer(manager.m_hTimerQueue, timerHandle, INVALID_HANDLE_VALUE);
                    MY_DEBUG_ASSERT(deletedOk == TRUE);
                }
            }
        };

        InvokeAfterHandle getNextTimerId()
        {
            const auto id = m_nextTimerStateId.fetch_add(1);
            MY_DEBUG_ASSERT(id < std::numeric_limits<InvokeAfterHandle>::max());

            return id;
        }

        HANDLE m_hTimerQueue = nullptr;
        std::mutex m_mutex;
        IntrusiveList<TimerState> m_timerStateList;
        std::atomic<InvokeAfterHandle> m_nextTimerStateId = 1;
        std::atomic_bool m_isDisposed = false;
        const RuntimeObjectRegistration m_runtimeObjectRegistration;
    };

    /**
     */
    std::unique_ptr<ITimerManager> ITimerManager::createDefault()
    {
        return std::make_unique<Win32TimerManager>();
    }

}  // namespace my::async
