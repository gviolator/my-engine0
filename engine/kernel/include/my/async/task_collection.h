// #my_engine_source_header
#pragma once
#include <list>

#include "my/async/task_base.h"
#include "my/kernel/kernel_config.h"
#include "my/runtime/async_disposable.h"
#include "my/threading/spin_lock.h"

namespace my::async
{

    /**
     */
    class MY_KERNEL_EXPORT TaskCollection final : public IAsyncDisposable
    {
    public:
        TaskCollection();

        TaskCollection(const TaskCollection&) = delete;

        ~TaskCollection();

        TaskCollection& operator=(const TaskCollection&) = delete;

        bool isEmpty() const;

        template <typename T>
        void push(Task<T> task);

        Task<> disposeAsync() override
        {
            return awaitCompletionInternal(true);
        }

        Task<> awaitCompletion()
        {
            return awaitCompletionInternal(false);
        }

    private:
#if !MY_TASK_COLLECTION_DEBUG
        using TaskEntry = CoreTaskPtr;
#else
        struct TaskEntry
        {
            CoreTaskPtr task;
            std::vector<void*> stack;

            TaskEntry(CoreTaskPtr);

            bool operator==(const CoreTask*) const;
        };

#endif

        void pushInternal(CoreTaskPtr);

        Task<> awaitCompletionInternal(bool dispose);


        mutable threading::SpinLock m_mutex;

        std::list<TaskEntry> m_tasks;
        TaskSource<> m_closeAwaiter = nullptr;
        bool m_isDisposing = false;
        bool m_isDisposed = false;
    };

    template <typename T>
    void TaskCollection::push(Task<T> task)
    {
        if (!task || task.isReady())
        {
            return;
        }

        pushInternal(std::move(static_cast<CoreTaskPtr&>(task)));
    }

}  // namespace my::async
