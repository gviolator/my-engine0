// #my_engine_source_header

#pragma once

#include <concepts>
#include <type_traits>

#include "my/kernel/kernel_config.h"
#include "my/diag/check.h"
#include "my/memory/mem_base.h"
//#include "my/rtti/type_info.h"
#include "my/rtti/rtti_object.h"
#include "my/rtti/ptr.h"

namespace my
{

    struct MY_ABSTRACT_TYPE IMemAllocator : IRefCounted
    {
        MY_INTERFACE(my::IMemAllocator, IRefCounted)

        [[nodiscard]] virtual void* alloc(size_t size) = 0;

        [[nodiscard]] virtual void* realloc(void* oldPtr, size_t size) = 0;

        virtual void free(void* ptr) = 0;

        virtual size_t getAllocationAlignment() const = 0;

        virtual void setName(const char*)
        {}

        virtual const char* getName() const
        {
            return "";
        }
    };

    struct MY_ABSTRACT_TYPE IAlignedMemAllocator : IMemAllocator
    {
        MY_INTERFACE(my::IAlignedMemAllocator, IMemAllocator)

        [[nodiscard]] virtual void* allocAligned(size_t size, size_t alignment) = 0;

        [[nodiscard]] virtual void* reallocAligned(void* oldPtr, size_t size, size_t alignment) = 0;

        virtual void freeAligned(void* ptr, size_t alignment) = 0;
    };

    using MemAllocatorPtr = my::Ptr<IMemAllocator>;

#if 0

    template <typename T>
    concept MemAllocatorProvider = requires {
        { T::getAllocator() } -> std::assignable_from<IMemAllocator&>;
    };

    /*
     *
     */
    template <typename T, MemAllocatorProvider AllocProvider>
    class StatelessStdAllocator
    {
    public:
        using Provider = AllocProvider;

        using value_type = T;
        using propagate_on_container_move_assignment = std::true_type;

        StatelessStdAllocator() noexcept = default;

        template <typename U, MemAllocatorProvider UProvider>
        StatelessStdAllocator(const StatelessStdAllocator<U, UProvider>&) noexcept
        {
        }

        [[nodiscard]]
        constexpr T* allocate(size_t n)
        {
            void* const ptr = Provider::getAllocator().realloc(nullptr, sizeof(T) * n);
            return reinterpret_cast<T*>(ptr);
        }

        void deallocate(T* p, [[maybe_unused]] std::size_t n)
        {
            // const size_t size = sizeof(T) * n;
            Provider::getAllocator().free(reinterpret_cast<void*>(p));
        }

        template <typename U, MemAllocatorProvider UProvider>
        bool operator==(const StatelessStdAllocator<U, UProvider>&) const
        {
            const IMemAllocator& a1 = UProvider::GetAllocator();
            const IMemAllocator& a2 = Provider::GetAllocator();

            return &a1 == &a2;
        }
    };

    /*
     */
    template <typename T>
    class MemAllocatorStdWrapper
    {
    public:
        using value_type = T;
        using propagate_on_container_move_assignment = std::true_type;

        MemAllocatorStdWrapper(IMemAllocator& alloc) noexcept :
            m_allocator(std::move(alloc))
        {
            G_ASSERT(m_allocator);
        }

        // move is forbidden: because original (other) allocator should always refer to valid allocator (will use it to free its own state)
        MemAllocatorStdWrapper(const MemAllocatorStdWrapper<T>& other) noexcept :
            m_allocator(other.m_allocator)
        {
            G_ASSERT(m_allocator);
        }

        template <typename U,
                  std::enable_if_t<!std::is_same_v<U, T>, int> = 0>
        MemAllocatorStdWrapper(const MemAllocatorStdWrapper<U>& other) noexcept :
            m_allocator(other.m_allocator)
        {
            G_ASSERT(m_allocator);
        }

        MemAllocatorStdWrapper<T>& operator=([[maybe_unused]] const MemAllocatorStdWrapper<T>& other) noexcept
        {
            G_ASSERT(m_allocator.get() == other.m_allocator.get());
            return *this;
        }

        template <typename U,
                  std::enable_if_t<!std::is_same_v<U, T>, int> = 0>
        MemAllocatorStdWrapper<T>& operator=([[maybe_unused]] const MemAllocatorStdWrapper<U>& other) noexcept
        {
            G_ASSERT(m_allocator.get() == other.m_allocator.Get());
            return *this;
        }

        [[nodiscard]]
        constexpr T* allocate(size_t n)
        {
            void* const ptr = m_allocator->realloc(nullptr, sizeof(T) * n);
            return reinterpret_cast<T*>(ptr);
        }

        void deallocate(T* p, [[maybe_unused]] std::size_t n)
        {
            // const size_t size = sizeof(T) * n;
            m_allocator->free(reinterpret_cast<void*>(p));
        }

        template <typename U>
        bool operator==(const MemAllocatorStdWrapper<U>& other) const
        {
            return m_allocator.get() == other.get();
        }

        const IMemAllocator::Ptr m_allocator;
    };

#endif

    MY_KERNEL_EXPORT IAlignedMemAllocator& getSystemAllocator();

    //MY_KERNEL_EXPORT MemAllocatorPtr createSystemAllocator(bool threadSafe);

//    MY_KERNEL_EXPORT IMemAllocator& getDefaultAllocator();

}  // namespace my
