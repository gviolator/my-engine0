// #my_engine_source_file


#pragma once

#include <optional>

#include "my/async/core/core_task.h"
#include "my/async/executor.h"
#include "my/diag/assert.h"
#include "my/diag/logging.h"
#include "my/kernel/kernel_config.h"
#include "my/utils/result.h"
#include "my/utils/scope_guard.h"

namespace my::async
{

    template <typename>
    class TaskBase;

    template <typename>
    class TaskSourceBase;

    template <typename>
    class TaskTryWrapper;

    template <typename T = void>
    class Task;

    template <typename T = void>
    class TaskSource;

}  // namespace my::async

namespace my::async_detail
{

    template <typename>
    struct TaskClientData;

    template <>
    struct TaskClientData<void>
    {
        bool taskDetached = false;
        bool taskGivenOut = false;
    };

    template <typename T>
    struct TaskClientData : TaskClientData<void>
    {
        std::optional<T> result;

        TaskClientData()
        {
            MY_DEBUG_ASSERT(reinterpret_cast<uintptr_t>(this) % alignof(TaskClientData<T>) == 0);
        }
    };

    /**

    */
    template <typename T>
    class TaskStateHolder : public async::CoreTaskPtr
    {
    public:
        TaskStateHolder(const TaskStateHolder<T>&) = delete;
        TaskStateHolder(TaskStateHolder<T>&&) noexcept = default;
        TaskStateHolder& operator=(const TaskStateHolder&) = delete;
        TaskStateHolder& operator=(TaskStateHolder&&) = default;

        bool isReady() const
        {
            MY_DEBUG_ASSERT(static_cast<bool>(*this), "Task is stateless");
            return this->getCoreTask().isReady();
        }

    protected:
        TaskStateHolder() = default;

        TaskStateHolder(async::CoreTaskPtr&& coreTask) :
            async::CoreTaskPtr(std::move(coreTask))
        {
        }

        const TaskClientData<T>& getClientData() const&
        {
            const void* const ptr = getCoreTask().getData();
            return *reinterpret_cast<const TaskClientData<T>*>(ptr);
        }

        TaskClientData<T>& getClientData() &
        {
            void* const ptr = this->getCoreTask().getData();
            return *reinterpret_cast<TaskClientData<T>*>(ptr);
        }

        TaskClientData<T>&& GetClientData() &&
        {
            void* const ptr = this->getCoreTask().getData();
            return std::move(*reinterpret_cast<TaskClientData<T>*>(ptr));
        }
    };

}  // namespace my::async_detail

namespace my::async
{

#pragma region Task

    /**
     */
    template <typename T>
    class TaskBase : public async_detail::TaskStateHolder<T>
    {
    public:
        using ValueType = T;

        static Task<T> makeRejected(Error::Ptr) noexcept;

        static Task<T> fromCoreTask(CoreTask::Ptr);

        TaskBase() = default;
        TaskBase(const TaskBase<T>&) = delete;
        TaskBase(TaskBase<T>&&) noexcept = default;
        TaskBase<T>& operator=(const TaskBase<T>&) = delete;
        TaskBase<T>& operator=(TaskBase<T>&&) = default;

        TaskTryWrapper<T> doTry();

        void rethrow() const
        {
            this->getCoreTask().rethrow();
        }

        Error::Ptr getError() const
        {
            MY_DEBUG_ASSERT(*this, "Task is stateless");
            if (!*this)
            {
                return nullptr;
            }

            return this->getCoreTask().getError();
        }

        /**
         */
        void setContinueOnCapturedExecutor(bool continueOnCapturedExecutor)
        {
            this->getCoreTask().setContinueOnCapturedExecutor(continueOnCapturedExecutor);
        }

        /**
         */
        bool isContinueOnCapturedExecutor() const
        {
            return this->getCoreTask().isContinueOnCapturedExecutor();
        }

        bool isRejected() const
        {
            return static_cast<bool>(this->getCoreTask().getError());
        }

        Task<T>& detach() &
        {
            MY_DEBUG_ASSERT(!this->getClientData().taskDetached, "Task already detached");
            this->getClientData().taskDetached = true;

            return static_cast<Task<T>&>(*this);
        }

        Task<T>&& detach() &&
        {
            MY_DEBUG_ASSERT(!this->getClientData().taskDetached, "Task already detached");
            this->getClientData().taskDetached = true;

            return std::move(static_cast<Task<T>&>(*this));
        }

        Result<T> asResult() const&
        {
            MY_DEBUG_ASSERT(this);
            MY_DEBUG_ASSERT(this->isReady());

            if (auto error = this->getCoreTask().getError())
            {
                return error;
            }

            if constexpr (std::is_same_v<void, T>)
            {
                if (!isRejected())
                {
                    return Result<>{};
                }
            }
            else
            {
                if (auto& value = this->getClientData().result; value)
                {
                    return *value;
                }
            }

            return {};
        }

        Result<T> asResult() &&
        {
            scope_on_leave
            {
                *this = TaskBase<T>{};
            };

            MY_DEBUG_ASSERT(this);
            MY_DEBUG_ASSERT(this->isReady());

            if (auto error = this->getCoreTask().getError())
            {
                return error;
            }

            if constexpr (std::is_same_v<void, T>)
            {
                if (!isRejected())
                {
                    return Result<>{};
                }
            }
            else
            {
                if (auto& value = this->getClientData().result; value)
                {
                    return Result<T>{std::move(*value)};
                }
            }

            return {};
        }

    protected:
        TaskBase(async::CoreTaskPtr&& coreTask) :
            async_detail::TaskStateHolder<T>(std::move(coreTask))
        {
        }

        ~TaskBase()
        {
            if (!static_cast<bool>(*this))
            {
                return;
            }

#if MY_DEBUG_ASSERT_ENABLED
            [[maybe_unused]]
            const bool taskReadyOrDetached = this->isReady() || this->getClientData().taskDetached || this->getCoreTask().hasContinuation();
            MY_DEBUG_ASSERT(taskReadyOrDetached, "Not finished Async::Task<> is leaving its scope. Use co_await or set continuation.");
#endif  // MY_DEBUG_ASSERT_ENABLED
        }
    };

    /**
     */
    template <typename T>
    class [[nodiscard]] Task final : public TaskBase<T>
    {
    public:
        template <typename... Args>
        static Task<T> makeResolved(Args&&...)
        requires(std::is_constructible_v<T, Args...>);

        Task() = default;
        Task(const Task<T>&) = delete;
        Task(Task<T>&&) = default;
        Task<T>& operator=(const Task<T>&) = delete;
        Task<T>& operator=(Task<T>&&) = default;

        Task<T>& operator=(std::nullptr_t) noexcept
        {
            static_cast<CoreTaskPtr&>(*this).operator=(nullptr);
            return *this;
        }

        const T& result() const&
        requires(std::is_copy_assignable_v<T>)
        {
            // static_assert(std::is_copy_assignable_v<T>);

            MY_DEBUG_ASSERT(this->isReady(), "Task<T> is not ready");
            this->rethrow();

            MY_DEBUG_ASSERT(this->getClientData().result);
            return *this->getClientData().result;
        }

        T result() &
        requires(std::is_copy_assignable_v<T>)
        {
            // static_assert(std::is_copy_assignable_v<T>);

            MY_DEBUG_ASSERT(this->isReady(), "Task<T> is not ready");
            this->rethrow();

            MY_DEBUG_ASSERT(this->getClientData().result);
            return *this->getClientData().result;
        }

        T result() &&
        {
            MY_DEBUG_ASSERT(this->isReady(), "Task<T> is not ready");

            scope_on_leave
            {
                this->reset();
            };

            this->rethrow();
            MY_DEBUG_ASSERT(this->getClientData().result);
            return std::move(*this->getClientData().result);
        }

        T operator*() &
        {
            return this->result();
        }

        T operator*() &&
        {
            return std::move(*this).result();
        }

    private:
        Task(async::CoreTaskPtr&& coreTask) :
            TaskBase<T>(std::move(coreTask))
        {
        }

        friend class TaskSourceBase<T>;
        friend class TaskBase<T>;
    };

    /**
     */
    template <>
    class [[nodiscard]] Task<void> final : public TaskBase<void>
    {
    public:
        static Task<void> makeResolved();
        static Task<void> makeUninitialized();

        Task() = default;
        Task(const Task<>&) = delete;
        Task(Task<>&&) = default;
        Task<>& operator=(const Task<>&) = delete;
        Task<>& operator=(Task<>&&) = default;
        Task<>& operator=(std::nullptr_t) noexcept
        {
            static_cast<CoreTaskPtr&>(*this).operator=(nullptr);
            return *this;
        }

        void result() const&
        {
            this->rethrow();
        }

        void result() &&
        {
            scope_on_leave
            {
                reset();
            };

            rethrow();
        }

    private:
        Task(async::CoreTaskPtr&& coreTask) :
            TaskBase<void>(std::move(coreTask))
        {
        }

        friend class TaskSourceBase<void>;
        friend class TaskBase<void>;
    };

    /**
     */
    template <typename T>
    class TaskTryWrapper
    {
    public:
        TaskTryWrapper() = default;

        TaskTryWrapper(CoreTask::Ptr coreTask) :
            m_coreTask(std::move(coreTask))
        {
            MY_DEBUG_ASSERT(m_coreTask);
        }

        explicit operator bool()
        {
            return static_cast<bool>(m_coreTask);
        }

        CoreTask::Ptr getCoreTaskPtr() &
        {
            return m_coreTask;
        }

        CoreTask::Ptr&& getCoreTaskPtr() &&
        {
            return std::move(m_coreTask);
        }

    private:
        CoreTask::Ptr m_coreTask;
    };

    template <typename T>
    TaskTryWrapper<T> TaskBase<T>::doTry()
    {
        return TaskTryWrapper<T>{*this};
    }

#pragma endregion

#pragma region TaskSource

    /**
     */
    template <typename T>
    class TaskSourceBase : public async_detail::TaskStateHolder<T>
    {
    public:
        static TaskSource<T> fromCoreTask(CoreTaskPtr coreTask);

        TaskSourceBase(std::nullptr_t)
        {
        }
        TaskSourceBase(TaskSourceBase<T>&&) = default;
        TaskSourceBase(const TaskSourceBase<T>&) = delete;
        TaskSourceBase& operator=(const TaskSourceBase<T>&) = delete;
        TaskSourceBase& operator=(TaskSourceBase<T>&&) = default;

        bool reject(Error::Ptr error) noexcept
        {
            return this->getCoreTask().tryRejectWithError(std::move(error));
        }

        Task<T> getTask()
        {
            MY_DEBUG_ASSERT(!this->getClientData().taskGivenOut, "Task<T> already takeout from source");

            this->getClientData().taskGivenOut = true;
            CoreTaskPtr coreTask = static_cast<CoreTaskPtr&>(*this);
            return Task<T>{std::move(coreTask)};
        }

    protected:
        TaskSourceBase(CoreTaskPtr coreTask) :
            async_detail::TaskStateHolder<T>{std::move(coreTask)}
        {
        }

        TaskSourceBase() :
            TaskSourceBase{createCoreTask<async_detail::TaskClientData<T>>()}
        {
        }

        virtual ~TaskSourceBase()
        {
            if (static_cast<bool>(*this) && !this->isReady())
            {
                this->reject(MakeError("TaskSource is destroyed with no result"));
            }
        }
    };

    /**
     */
    template <typename T>
    class TaskSource final : public TaskSourceBase<T>
    {
    public:
        using TaskSourceBase<T>::TaskSourceBase;
        using TaskSourceBase<T>::fromCoreTask;

        template <typename... Args>
        bool resolve(Args&&... args)
        requires(std::is_constructible_v<T, Args...>)
        {
            static_assert(std::is_constructible_v<T, Args...>);

            return this->getCoreTask().tryResolve([&](CoreTask::Rejector&) noexcept
            {
                this->getClientData().result.emplace(std::forward<Args>(args)...);
            });
        }

        friend class TaskSourceBase<T>;
    };

    /**
     */
    template <>
    class TaskSource<void> final : public TaskSourceBase<void>
    {
    public:
        using TaskSourceBase<void>::TaskSourceBase;
        using TaskSourceBase<void>::fromCoreTask;

        bool resolve()
        {
            return this->getCoreTask().tryResolve();
        }

        friend class TaskSourceBase<void>;
    };

#pragma endregion

    template <typename T>
    Task<T> TaskBase<T>::makeRejected(Error::Ptr error) noexcept
    {
        MY_DEBUG_ASSERT(error);
        if (!error)
        {
            return {};
        }

        TaskSource<T> taskSource;
        taskSource.reject(std::move(error));
        return taskSource.getTask();
    }

    template <typename T>
    Task<T> TaskBase<T>::fromCoreTask(CoreTask::Ptr coreTask)
    {
        MY_DEBUG_ASSERT(coreTask);
        return Task<T>{std::move(coreTask)};
    }

    template <typename T>
    template <typename... Args>
    Task<T> Task<T>::makeResolved(Args&&... args)
    requires(std::is_constructible_v<T, Args...>)
    {
        static_assert(std::is_constructible_v<T, Args...>);

        TaskSource<T> taskSource;
        taskSource.resolve(std::forward<Args>(args)...);
        return taskSource.getTask();
    }

    inline Task<void> Task<void>::makeResolved()
    {
        TaskSource<> taskSource;
        taskSource.resolve();
        return taskSource.getTask();
    }

    inline Task<void> Task<void>::makeUninitialized()
    {
        return Task<>{nullptr};
    }

    template <typename T>
    TaskSource<T> TaskSourceBase<T>::fromCoreTask(CoreTaskPtr coreTask)
    {
        return TaskSource<T>{std::move(coreTask)};
    }

    template <typename T>
    Task<T> makeResolvedTask(T&& result)
    {
        return Task<T>::makeResolved(std::forward<T>(result));
    }

    inline Task<> makeResolvedTask()
    {
        return Task<>::makeResolved();
    }

}  // namespace my::async
