// #my_engine_source_file
#pragma once

#include "my/async/core/core_task.h"
#include "my/async/task_base.h"
#include "my/kernel/kernel_config.h"

namespace my::async_detail
{
    using TaskContainerIterator = async::CoreTaskPtr (*)(void*) noexcept;

    /**
     */
    class MY_KERNEL_EXPORT CoreTaskLinkedList
    {
    public:
        class iterator
        {
        public:
            using iterator_category = std::input_iterator_tag;
            using value_type = async::CoreTask*;
            using difference_type = ptrdiff_t;
            using pointer = std::add_pointer_t<value_type>;
            using reference = std::add_lvalue_reference<value_type>;

            iterator& operator++();
            iterator operator++(int);
            async::CoreTask* operator*() const;

        private:
            iterator(async::CoreTask* = nullptr);

            async::CoreTask* m_taskPtr;

            friend bool operator==(const iterator&, const iterator&);
            friend bool operator!=(const iterator&, const iterator&);
            friend class CoreTaskLinkedList;
        };

        template <typename T>
        static CoreTaskLinkedList fromContainer(T iterBegin, T iterEnd);

        template <typename T, typename... U>
        static CoreTaskLinkedList fromTasks(async::Task<T>&, async::Task<U>&...);

        CoreTaskLinkedList(TaskContainerIterator, void* data);
        ~CoreTaskLinkedList();
        CoreTaskLinkedList(CoreTaskLinkedList&&);
        CoreTaskLinkedList(const CoreTaskLinkedList&) = delete;

        CoreTaskLinkedList& operator=(const CoreTaskLinkedList&) = delete;
        CoreTaskLinkedList& operator=(CoreTaskLinkedList&&);

        iterator begin();
        iterator end();
        size_t size() const;
        bool empty() const;
        void reset();
        void append(async::CoreTaskPtr);

    private:
        template <typename... T>
        static std::array<async::CoreTaskPtr, sizeof...(T)> makeCoreTaskPtrArray(async::Task<T>&... tasks)
        {
            return std::array{static_cast<async::CoreTaskPtr&>(tasks)...};
        }

        async::CoreTask* m_head = nullptr;
        size_t m_size = 0;
    };

    template <typename T>
    CoreTaskLinkedList CoreTaskLinkedList::fromContainer(T iterBegin, T iterEnd)
    {
        struct State
        {
            T current;
            T end;
        };

        const auto iteratorFunc = [](void* data) noexcept -> async::CoreTaskPtr
        {
            using namespace my::async;

            auto& state = *reinterpret_cast<State*>(data);

            if(state.current == state.end)
            {
                return nullptr;
            }

            decltype(auto) taskOrPtr = *state.current;  //
            CoreTaskPtr coreTask;

            if constexpr(std::is_pointer_v<std::remove_reference_t<decltype(taskOrPtr)>>)
            {
                MY_DEBUG_ASSERT(taskOrPtr != nullptr, "Container<Task*> can not contains nullptr");
                coreTask = static_cast<CoreTaskPtr&>(*taskOrPtr);
            }
            else
            {
                coreTask = static_cast<CoreTaskPtr&>(taskOrPtr);
            }

            ++state.current;

            return coreTask;
        };

        State state = {iterBegin, iterEnd};

        return CoreTaskLinkedList{iteratorFunc, &state};
    }

    template <typename T, typename... U>
    CoreTaskLinkedList CoreTaskLinkedList::fromTasks(async::Task<T>& task, async::Task<U>&... tail)
    {
        auto coreTaskPtrArray = CoreTaskLinkedList::makeCoreTaskPtrArray(task, tail...);
        return CoreTaskLinkedList::fromContainer(std::begin(coreTaskPtrArray), std::end(coreTaskPtrArray));
    }

}  // namespace my::async_detail
