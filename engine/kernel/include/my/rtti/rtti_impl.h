// #my_engine_source_file

#pragma once
#include <concepts>
#include <cstddef>
#include <thread>
#include <type_traits>

#include "my/diag/check.h"
#include "my/memory/mem_allocator.h"
#include "my/rtti/ptr.h"
#include "my/rtti/rtti_object.h"
#include "my/rtti/rtti_utils.h"
#include "my/utils/tuple_utility.h"

#if MY_DEBUG && defined(MY_DEBUG_CHECK_ENABLED)
    #define MY_RTTI_VALIDATE_SHARED_STATE
#endif

namespace my
{
    template <typename T>
    concept RefCountedClassWithImplTag = requires {
        typename T::RcClassImplTag;
    } && std::is_same_v<typename T::RcClassImplTag::Type, T>;

}  // namespace my

namespace my::rtti_detail
{
    template <typename T>
    struct MyRcClassImplTag
    {
        using Type = T;
    };

#ifdef MY_RTTI_VALIDATE_SHARED_STATE
    // "Class Marker" is used to validate memory where shared state expected to be allocated.
    // This is need to be ensure that ref counted class created by any rtti factory function
    // with is used special memory layout to place [shared state][instance].
    // Without that the default implementation (with MY_CLASS) of the ref counted class will not be operable.
    // 6004214524017983822 == ['M', 'Y', '_', 'C', 'L', 'A', 'S', 'S']
    inline constexpr uint64_t MyClassMarkerValue = 6004214524017983822;
#endif
    class MY_KERNEL_EXPORT RttiClassSharedState final : public my::IWeakRef
    {
    public:
        using AcquireFunc = IRefCounted* (*)(void*);
        using DestructorFunc = void (*)(void*);

        RttiClassSharedState(const RttiClassSharedState&) = delete;

        RttiClassSharedState(MemAllocator* allocator_, AcquireFunc, DestructorFunc, std::byte* ptr);

        RttiClassSharedState operator=(const RttiClassSharedState&) = delete;

        void addInstanceRef();

        void releaseInstanceRef();

        uint32_t getInstanceRefsCount() const;

        IWeakRef* acquireWeakRef();

        MemAllocator* getAllocator() const;

    private:
        void releaseStorageRef();

        void addWeakRef() override;

        void releaseRef() override;

        IRefCounted* acquire() override;

        bool isDead() const override;

#ifdef MY_RTTI_VALIDATE_SHARED_STATE
    public:
        const uint64_t m_classMarker = MyClassMarkerValue;

    private:
#endif

        MemAllocator* m_allocator;
        AcquireFunc m_acquireFunc;
        DestructorFunc m_destructorFunc;
        std::byte* const m_allocatedPtr;
        std::atomic<uint32_t> m_stateCounter{1ui32};
        std::atomic<uint32_t> m_instanceCounter{1ui32};
    };
    /**

    */
    // template <RefCountedConcept RC>
    struct MY_KERNEL_EXPORT RttiClassStorage
    {
        using AcquireFunc = RttiClassSharedState::AcquireFunc;
        using DestructorFunc = RttiClassSharedState::DestructorFunc;
        using SharedState = RttiClassSharedState;

        // static constexpr size_t BlockAlignment = alignof(std::max_align_t);
        static constexpr size_t SharedStateSize = sizeof(AlignedStorage<sizeof(SharedState), alignof(SharedState)>);

        template <typename T>
        using ValueStorage = AlignedStorage<sizeof(T), std::max(alignof(SharedState), alignof(T))>;

        // additional alignof(T) = extra space for cases when alignment fixed by offset
        template <typename T>
        static constexpr size_t InstanceStorageSize = SharedStateSize + sizeof(ValueStorage<T>) + alignof(T);

        template <typename T>
        using InstanceInplaceStorage = AlignedStorage<InstanceStorageSize<T>, alignof(T)>;

        template <typename T>
        static SharedState& getSharedState(const T& instance)
        {
            const std::byte* const instancePtr = reinterpret_cast<const std::byte*>(&instance);
            std::byte* const statePtr = const_cast<std::byte*>(instancePtr - SharedStateSize);

            auto& sharedState = *reinterpret_cast<SharedState*>(statePtr);

#ifdef MY_RTTI_VALIDATE_SHARED_STATE
            // check that class instance has valid shared state with expected layout.
            MY_DEBUG_FATAL(sharedState.m_classMarker == MyClassMarkerValue, "Invalid SharedState. RefCounted class must be created only with rtti instance factory functions");
#endif
            return sharedState;
        }

        static void* allocateStateAndInstance(void* inplaceMemBlock, MemAllocator* allocator, size_t size, size_t alignment, AcquireFunc, DestructorFunc);

        static void* getInstancePtr(SharedState& state);
        // {
        //     std::byte* const statePtr = reinterpret_cast<std::byte*>(&state);
        //     return statePtr + SharedStateSize;
        // }

        template <typename T>
        static T& getInstance(SharedState& state)
        {
            // std::byte* const statePtr = reinterpret_cast<std::byte*>(&state);
            // std::byte* const instancePtr = statePtr + SharedStateSize;

            return *reinterpret_cast<T*>(getInstancePtr(state));
        }

        template <typename T, typename... Args>
        static T* instanceFactory(void* inplaceMemBlock, MemAllocator* allocator, Args&&... args)
        {
            static_assert(RefCountedClassWithImplTag<T>, "Class expected to be implemented with MY_CLASS/MY_REFCOUNTED_CLASS/MY_IMPLEMENT_REFCOUNTED. Please, check Class declaration");
            static_assert((alignof(T) <= alignof(SharedState)) || (alignof(T) % alignof(SharedState) == 0), "Unsupported type alignment.");
            MY_DEBUG_CHECK(static_cast<bool>(inplaceMemBlock) != static_cast<bool>(allocator));

            const auto acquire = [](void* instancePtr) -> IRefCounted*
            {
                T* const instance = reinterpret_cast<T*>(instancePtr);
                auto const refCounted = rtti::staticCast<IRefCounted*>(instance);
                MY_DEBUG_CHECK(refCounted, "Runtime cast ({}) -> IRefCounted failed", rtti::getTypeInfo<T>().getTypeName());

                return refCounted;
            };

            const auto destructor = [](void* instancePtr)
            {
                T* instance = reinterpret_cast<T*>(instancePtr);
                std::destroy_at(instance);
            };

            void* const instanceMemPtr = allocateStateAndInstance(inplaceMemBlock, allocator, sizeof(T), alignof(T), acquire, destructor);
            MY_FATAL(instanceMemPtr);

            return new(instanceMemPtr) T(std::forward<Args>(args)...);

            //             constexpr size_t StorageSize = InstanceStorageSize<T>;

            //             // Allocator or preallocated memory
            //             std::byte* const storage = reinterpret_cast<std::byte*>(allocator ? allocator->alloc(StorageSize) : inplaceMemBlock);
            //             MY_DEBUG_FATAL(storage);
            //             MY_DEBUG_FATAL(reinterpret_cast<uintptr_t>(storage) % alignof(SharedState) == 0);

            //             std::byte* statePtr = storage;
            //             std::byte* instancePtr = statePtr + SharedStateSize;

            //             // need to respect type's alignment:
            //             // if storage is not properly aligned there is need to offset state and instance pointers:
            //             if (const uintptr_t alignmentOffset = reinterpret_cast<uintptr_t>(instancePtr) % alignof(T); alignmentOffset > 0)
            //             {
            //                 const size_t offsetGap = alignof(T) - alignmentOffset;
            //                 statePtr = statePtr + offsetGap;
            //                 instancePtr = instancePtr + offsetGap;

            //                 MY_DEBUG_FATAL(reinterpret_cast<uintptr_t>(statePtr) % alignof(SharedState) == 0);
            //                 MY_DEBUG_FATAL(reinterpret_cast<uintptr_t>(instancePtr) % alignof(T) == 0);
            //                 MY_DEBUG_FATAL(StorageSize >= SharedStateSize + sizeof(T) + offsetGap);
            //             }

            //             MY_DEBUG_FATAL(reinterpret_cast<uintptr_t>(instancePtr) % alignof(T) == 0, "Invalid address, expected alignment ({})", alignof(T));
            // #if MY_DEBUG
            //             memset(storage, 0, StorageSize);
            // #endif
            //             [[maybe_unused]]
            //             auto sharedState = new(statePtr) SharedState(std::move(allocator), acquire, destructor, storage);  // -V799

            //             return new(instancePtr) T(std::forward<Args>(args)...);
        }

        template <typename T, typename MemBlock, typename... Args>
        static T* createInstanceInplace(MemBlock& memBlock, Args&&... args)
        {
            static_assert(sizeof(MemBlock) >= InstanceStorageSize<T>);

            void* const ptr = reinterpret_cast<void*>(&memBlock);
            return instanceFactory<T>(ptr, nullptr, std::forward<Args>(args)...);
        }

        template <typename T, typename... Args>
        static T* createInstanceWithAllocator(MemAllocator* allocator, Args&&... args)
        {
            if (!allocator)
            {
                allocator = &getSystemAllocator();
            }

            return instanceFactory<T>(nullptr, allocator, std::forward<Args>(args)...);
        }

        template <typename T, typename... Args>
        static T* createInstance(Args&&... args)
        {
            return instanceFactory<T>(nullptr, &getSystemAllocator(), std::forward<Args>(args)...);
        }
    };

    // template <RefCountedConcept RC>
   

}  // namespace my::rtti_detail

namespace my::rtti
{
    /*  template <RefCountedConcept RCPolicy>
      struct RttiClassPolicy
      {
          using RC = RCPolicy;
      };

      struct RCPolicy
      {
          using Concurrent = ::my::ConcurrentRC;
          using StrictSingleThread = ::my::StrictSingleThreadRC;
      };*/

    template <typename T>
    inline constexpr size_t InstanceStorageSize = rtti_detail::RttiClassStorage::template InstanceStorageSize<T>;

    template <typename T>
    using InstanceInplaceStorage = typename rtti_detail::RttiClassStorage::template InstanceInplaceStorage<T>;

    template <typename ClassImpl, typename Itf = ClassImpl, typename MemBlock, typename... Args>
    Ptr<Itf> createInstanceInplace(MemBlock& memBlock, Args&&... args)
    {
        using namespace ::my::rtti_detail;

        ClassImpl* const instance = RttiClassStorage::template createInstanceInplace<ClassImpl>(memBlock, std::forward<Args>(args)...);
        auto const itf = rtti::staticCast<Itf*>(instance);
        MY_DEBUG_CHECK(itf);
        return rtti::TakeOwnership(itf);
    }

    template <typename ClassImpl, typename Itf = ClassImpl, typename... Args>
    Ptr<Itf> createInstanceSingleton(Args&&... args)
    {
        static rtti::InstanceInplaceStorage<ClassImpl> storage;
        return rtti::createInstanceInplace<ClassImpl, Itf>(storage, std::forward<Args>(args)...);
    }

    template <typename ClassImpl, typename Interface_ = ClassImpl, typename... Args>
    // requires ComInterface<Interface_>
    Ptr<Interface_> createInstanceWithAllocator(MemAllocator* allocator, Args&&... args)
    {
        using namespace ::my::rtti_detail;

        ClassImpl* const instance = RttiClassStorage::template createInstanceWithAllocator<ClassImpl>(allocator, std::forward<Args>(args)...);
        auto const itf = rtti::staticCast<Interface_*>(instance);
        MY_DEBUG_CHECK(itf);
        return rtti::TakeOwnership(itf);
    }

    template <typename ClassImpl, typename Interface_ = ClassImpl, typename... Args>
    // requires ComInterface<Interface_>
    Ptr<Interface_> createInstance(Args&&... args)
    {
        using namespace ::my::rtti_detail;

        ClassImpl* const instance = RttiClassStorage::template createInstance<ClassImpl>(std::forward<Args>(args)...);
        Interface_* const itf = rtti::staticCast<Interface_*>(instance);
        MY_DEBUG_CHECK(itf);
        return rtti::TakeOwnership(itf);
    }

}  // namespace my::rtti

#define MY_IMPLEMENT_RTTI_OBJECT                                             \
                                                                             \
public:                                                                      \
    using ::my::IRttiObject::is;                                             \
    using ::my::IRttiObject::as;                                             \
                                                                             \
    bool is(const ::my::rtti::TypeInfo& type) const noexcept override        \
    {                                                                        \
        return ::my::rtti::runtimeIs<decltype(*this)>(type);                 \
    }                                                                        \
                                                                             \
    void* as(const ::my::rtti::TypeInfo& type) noexcept override             \
    {                                                                        \
        return ::my::rtti::runtimeCast(*this, type);                         \
    }                                                                        \
                                                                             \
    const void* as(const ::my::rtti::TypeInfo& type) const noexcept override \
    {                                                                        \
        return ::my::rtti::runtimeCast(*this, type);                         \
    }

#define MY_IMPLEMENT_REFCOUNTED(ClassImpl)                                     \
public:                                                                        \
    using RttiClassStorage = ::my::rtti_detail::RttiClassStorage;              \
    using RcClassImplTag = ::my::rtti_detail::MyRcClassImplTag<ClassImpl>;     \
                                                                               \
    void addRef() override                                                     \
    {                                                                          \
        RttiClassStorage::getSharedState(*this).addInstanceRef();              \
    }                                                                          \
                                                                               \
    void releaseRef() override                                                 \
    {                                                                          \
        RttiClassStorage::getSharedState(*this).releaseInstanceRef();          \
    }                                                                          \
                                                                               \
    ::my::IWeakRef* getWeakRef() override                                      \
    {                                                                          \
        return RttiClassStorage::getSharedState(*this).acquireWeakRef();       \
    }                                                                          \
                                                                               \
    uint32_t getRefsCount() const override                                     \
    {                                                                          \
        return RttiClassStorage::getSharedState(*this).getInstanceRefsCount(); \
    }                                                                          \
                                                                               \
    const ::my::MemAllocator* getRttiClassInstanceAllocator() const           \
    {                                                                          \
        return RttiClassStorage::getSharedState(*this).getAllocator();         \
    }

#define MY_RTTI_CLASS(ClassImpl, ...) \
    MY_TYPEID(ClassImpl)              \
    MY_CLASS_BASE(__VA_ARGS__)        \
                                      \
    MY_IMPLEMENT_RTTI_OBJECT

#define MY_REFCOUNTED_CLASS(ClassImpl, ...) \
    MY_RTTI_CLASS(ClassImpl, __VA_ARGS__)   \
    MY_IMPLEMENT_REFCOUNTED(ClassImpl)

//#define MY_REFCOUNTED_CLASS_(ClassImpl, ...) MY_REFCOUNTED_CLASS(ClassImpl, __VA_ARGS__)

#define MY_CLASS_ALLOW_PRIVATE_CONSTRUCTOR \
    template <my::RefCountedConcept>       \
    friend struct my::rtti_detail::RttiClassStorage;

#ifdef MY_RTTI_VALIDATE_SHARED_STATE
    #undef MY_RTTI_VALIDATE_SHARED_STATE
#endif
