// #my_engine_source_file
#include "my/async/task_collection.h"

#include "my/diag/check.h"
#include "my/threading/lock_guard.h"

namespace my::async
{
    namespace
    {
        bool operator==(CoreTaskPtr& t, const CoreTask* taskPtr)
        {
            return getCoreTask(t) == taskPtr;
        }
    }  // namespace

    TaskCollection::TaskCollection() = default;

    TaskCollection::~TaskCollection()
    {
#if MY_DEBUG_CHECK_ENABLED
        lock_(m_mutex);
        MY_DEBUG_FATAL(!m_closeAwaiter);
        MY_DEBUG_FATAL(m_tasks.empty(), "All task collection must be be awaited");
#endif
    }

    bool TaskCollection::isEmpty() const
    {
        lock_(m_mutex);
        return m_tasks.empty();
    }

    void TaskCollection::pushInternal(CoreTaskPtr task)
    {
        MY_DEBUG_CHECK(task);
        if (!task)
        {
            return;
        }

        CoreTask* const coreTask = async::getCoreTask(task);
        MY_DEBUG_FATAL(coreTask);
        if (coreTask->isReady())
        {
            // Even if a task is not ready here, it may become so immediately after (or in the process of) placing it in the collection.
            // Therefore, the scope of the mutex lock must be limited.
            return;
        }

        {
            lock_(m_mutex);
            MY_DEBUG_CHECK(!m_isDisposed);
            m_tasks.emplace_back(std::move(task));
        }

        coreTask->setReadyCallback([](void* selfPtr, void* taskPtr) noexcept
        {
            MY_DEBUG_FATAL(selfPtr);
            MY_DEBUG_FATAL(taskPtr);
            auto& self = *reinterpret_cast<TaskCollection*>(selfPtr);
            auto completedTask = reinterpret_cast<CoreTask*>(taskPtr);

            lock_(self.m_mutex);

            auto& tasks = self.m_tasks;

            auto iter = std::find_if(tasks.begin(), tasks.end(), [completedTask](TaskEntry& task)
            {
                return task == completedTask;
            });
            MY_DEBUG_CHECK(iter != tasks.end());
            if (iter != tasks.end())
            {
                tasks.erase(iter);
            }

            if (tasks.empty() && self.m_closeAwaiter)
            {
                MY_DEBUG_CHECK(self.m_isDisposing);
                auto awaiter = std::move(self.m_closeAwaiter);
                awaiter.resolve();
            }
        }, this, coreTask);
    }

    Task<> TaskCollection::awaitCompletionInternal(bool dispose)
    {
        {
            lock_(m_mutex);
            const bool alreadyDisposing = std::exchange(m_isDisposing, true);

            MY_DEBUG_CHECK(!alreadyDisposing, "TaskCollection::disposeAsync called multiply times");
            if (alreadyDisposing)
            {
                co_return;
            }
        }

        scope_on_leave
        {
            lock_(m_mutex);
            MY_DEBUG_CHECK(m_isDisposing);
            m_isDisposing = false;
        };

        do
        {
            Task<> awaiterTask;

            {
                lock_(m_mutex);
                if (m_tasks.empty())
                {
                    if (dispose)
                    {
                        MY_DEBUG_CHECK(!m_isDisposed);
                        m_isDisposed = true;
                    }
                    break;
                }

                MY_DEBUG_CHECK(!m_closeAwaiter);
                m_closeAwaiter = {};
                awaiterTask = m_closeAwaiter.getTask();
            }

            co_await awaiterTask;
        } while (true);
    }

}  // namespace my::async
