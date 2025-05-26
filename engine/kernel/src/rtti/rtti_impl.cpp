// #my_engine_source_file
#include "my/rtti/rtti_impl.h"

namespace my::rtti_detail
{
    namespace
    {
        inline void addRef(std::atomic<uint32_t>& counter)
        {
            [[maybe_unused]] const auto value = counter.fetch_add(1, std::memory_order_release);
            MY_DEBUG_ASSERT(value > 0);
        }

        inline uint32_t removeRef(std::atomic<uint32_t>& counter)
        {
            const auto value = counter.fetch_sub(1, std::memory_order_release);
            MY_DEBUG_ASSERT(value > 0);
            return value;
        }

        bool tryAddRef(std::atomic<uint32_t>& counter)
        {
            uint32_t value = counter.load(std::memory_order_acquire);

            do
            {
                if (value == 0)
                {
                    return false;
                }
            } while (!counter.compare_exchange_weak(value, counter + 1));

            return true;
        }
    }  // namespace

    RttiClassSharedState::RttiClassSharedState(IMemAllocator* allocator, AcquireFunc acquire, DestructorFunc destructor, std::byte* allocatedPtr) :
        m_allocator(allocator),
        m_acquireFunc(acquire),
        m_destructorFunc(destructor),
        m_allocatedPtr(allocatedPtr)
    {
        MY_DEBUG_ASSERT(m_acquireFunc);
        MY_DEBUG_ASSERT(m_destructorFunc);
        MY_DEBUG_ASSERT(m_stateCounter.load(std::memory_order_relaxed) == 1);
        MY_DEBUG_ASSERT(m_instanceCounter.load(std::memory_order_relaxed) == 1);
    }

    IMemAllocator* RttiClassSharedState::getAllocator() const
    {
        return m_allocator;
    }

    void RttiClassSharedState::addInstanceRef()
    {
        addRef(m_instanceCounter);
        addRef(m_stateCounter);
    }

    void RttiClassSharedState::releaseInstanceRef()
    {
        if (removeRef(m_instanceCounter) == 1)
        {
            void* const instancePtr = RttiClassStorage::getInstancePtr(*this);
            m_destructorFunc(instancePtr);
        }

        releaseStorageRef();
    }

    uint32_t RttiClassSharedState::getInstanceRefsCount() const
    {
        return m_instanceCounter.load(std::memory_order_relaxed);
    }

    IWeakRef* RttiClassSharedState::acquireWeakRef()
    {
        addRef(m_stateCounter);
        return this;
    }

    void RttiClassSharedState::releaseStorageRef()
    {
        if (removeRef(m_stateCounter) == 1)
        {
            MY_DEBUG_ASSERT(m_stateCounter.load(std::memory_order_relaxed) == 0);
            MY_DEBUG_ASSERT(m_instanceCounter.load(std::memory_order_relaxed) == 0);

            std::destroy_at(this);

            auto allocator = std::exchange(m_allocator, nullptr);
            if (allocator)
            {
                allocator->free(m_allocatedPtr);
            }
        }
    }

    void RttiClassSharedState::addWeakRef()
    {
        addRef(m_stateCounter);
    }

    void RttiClassSharedState::releaseRef()
    {
        releaseStorageRef();
    }

    IRefCounted* RttiClassSharedState::acquire()
    {
        if (tryAddRef(m_instanceCounter))
        {
            addRef(m_stateCounter);
            void* const instancePtr = RttiClassStorage::getInstancePtr(*this);
            IRefCounted* const instance = m_acquireFunc(instancePtr);
            return instance;
        }

        return nullptr;
    }

    bool RttiClassSharedState::isDead() const
    {
        MY_DEBUG_ASSERT(m_stateCounter.load() > 0);
        return m_instanceCounter.load(std::memory_order_relaxed) == 0;
    }

    void* RttiClassStorage::allocateStateAndInstance(void* inplaceMemBlock, IMemAllocator* allocator, size_t instanceSize, size_t instanceAlignment, AcquireFunc acquireFunc, DestructorFunc destructorFunc)
    {
        // static_assert(RefCountedClassWithImplTag<T>, "Class expected to be implemented with MY_CLASS/MY_REFCOUNTED_CLASS/MY_IMPLEMENT_REFCOUNTED. Please, check Class declaration");
        // static_assert((alignof(T) <= alignof(SharedState)) || (alignof(T) % alignof(SharedState) == 0), "Unsupported type alignment.");
        MY_DEBUG_ASSERT(static_cast<bool>(inplaceMemBlock) != static_cast<bool>(allocator));

        // const auto acquire = [](void* instancePtr) -> IRefCounted*
        // {
        //     T* const instance = reinterpret_cast<T*>(instancePtr);
        //     auto const refCounted = rtti::staticCast<IRefCounted*>(instance);
        //     MY_DEBUG_ASSERT(refCounted, "Runtime cast ({}) -> IRefCounted failed", rtti::getTypeInfo<T>().getTypeName());

        //     return refCounted;
        // };

        // const auto destructor = [](void* instancePtr)
        // {
        //     T* instance = reinterpret_cast<T*>(instancePtr);
        //     std::destroy_at(instance);
        // };

        const size_t StorageSize = SharedStateSize + instanceSize + instanceAlignment;

        // Allocator or preallocated memory
        std::byte* const storage = reinterpret_cast<std::byte*>(allocator ? allocator->alloc(StorageSize) : inplaceMemBlock);
        MY_DEBUG_FATAL(storage);
        MY_DEBUG_FATAL(reinterpret_cast<uintptr_t>(storage) % alignof(SharedState) == 0);

        std::byte* statePtr = storage;
        std::byte* instanceMemPtr = statePtr + SharedStateSize;

        // need to respect type's alignment:
        // if storage is not properly aligned there is need to offset state and instance pointers:
        if (const uintptr_t alignmentOffset = reinterpret_cast<uintptr_t>(instanceMemPtr) % instanceAlignment; alignmentOffset > 0)
        {
            const size_t offsetGap = instanceAlignment - alignmentOffset;
            statePtr = statePtr + offsetGap;
            instanceMemPtr = instanceMemPtr + offsetGap;

            MY_DEBUG_FATAL(reinterpret_cast<uintptr_t>(statePtr) % alignof(SharedState) == 0);
            MY_DEBUG_FATAL(reinterpret_cast<uintptr_t>(instanceMemPtr) % instanceAlignment == 0);
            MY_DEBUG_FATAL(StorageSize >= SharedStateSize + instanceSize + offsetGap);
        }

        MY_DEBUG_FATAL(reinterpret_cast<uintptr_t>(instanceMemPtr) % instanceAlignment == 0, "Invalid address, expected alignment ({})", instanceAlignment);
#if MY_DEBUG
        memset(storage, 0, StorageSize);
#endif
        [[maybe_unused]]
        auto sharedState = new(statePtr) SharedState(std::move(allocator), acquireFunc, destructorFunc, storage);  // -V799

        return instanceMemPtr;
        // return new(instancePtr) T(std::forward<Args>(args)...);
    }

    void* RttiClassStorage::getInstancePtr(SharedState& state)
    {
        std::byte* const statePtr = reinterpret_cast<std::byte*>(&state);
        return statePtr + SharedStateSize;
    }

}  // namespace my::rtti_detail
