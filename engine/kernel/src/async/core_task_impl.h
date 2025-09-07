// #my_engine_source_file


#pragma once

#include <atomic>

#include "my/async/core/core_task.h"
#include "my/memory/allocator.h"

namespace my::async
{

    /**
     */
    class CoreTaskImpl final : public CoreTask
    {
    public:
        CoreTaskImpl(IAllocator&, void* allocatedStorage, size_t size, StateDestructorCallback destructor);

        ~CoreTaskImpl();

        // uintptr_t getTaskId() const override;
        void addRef() override;
        void releaseRef() override;
        bool isReady() const override;

        ErrorPtr getError() const override;
        const void* getData() const override;
        void* getData() override;
        size_t getDataSize() const override;
        void setContinuation(TaskContinuation) override;
        bool hasContinuation() const override;
        bool hasCapturedExecutor() const override;

        void setContinueOnCapturedExecutor(bool continueOnCapturedExecutor) override;
        bool isContinueOnCapturedExecutor() const override;

        void setReadyCallback(ReadyCallback callback, void*, void* = nullptr) override;
        bool tryResolveInternal(ResolverCallback, void*) override;

        CoreTaskImpl* getNext() const;
        void setNext(CoreTaskImpl*);

        std::string getName() const;
        void setName(std::string name);



    private:
        void invokeReadyCallback();
        void tryScheduleContinuation();

        IAllocator& m_allocator;

        // In some cases m_allocatedStorage can differ from (void*)this, because of custom types alignment.
        // For simplification aligned storage allocation, just keeps m_allocatedStorage (which may initially have incorrect alignment).
        void* const m_allocatedStorage;
        const size_t m_dataSize;
        const StateDestructorCallback m_destructor;
        std::atomic<uint32_t> m_refsCount{1};
        mutable std::atomic<uint32_t> m_flags{0};
        ErrorPtr m_error;
        TaskContinuation m_continuation;
        Executor::Invocation m_readyCallback;
        std::atomic<bool> m_isContinueOnCapturedExecutor = true;
        CoreTaskImpl* m_next = nullptr;
        std::string m_name = "";
    };

}  // namespace my::async
