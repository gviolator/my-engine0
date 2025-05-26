// #my_engine_source_file

#pragma once

#include <concepts>
#include <type_traits>

#include "my/diag/assert.h"
#include "my/kernel/kernel_config.h"
#include "my/memory/mem_base.h"

// #include "my/rtti/type_info.h"
#include "my/rtti/ptr.h"
#include "my/rtti/rtti_object.h"
#include "my/utils/type_utility.h"

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
        {
        }

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

    template <typename U>
    concept IsMemAllocatorRef = std::is_reference_v<U> && std::is_assignable_v<IMemAllocator&, U>;

    template <typename T>
    concept MemAllocatorProvider = requires {
        { T::get_allocator() } -> IsMemAllocatorRef;
    };

    /*
     *
     */
    template <typename T, MemAllocatorProvider AllocProvider>
    class StatelessStdAllocator
    {
    public:
        ///static_assert(MemAllocatorProvider<decltype(AllocProvider)>);
        using value_type = T;
        using propagate_on_container_move_assignment = std::true_type;

        StatelessStdAllocator() noexcept = default;

        template <typename U, MemAllocatorProvider UProvider>
        StatelessStdAllocator(const StatelessStdAllocator<U, UProvider>&) noexcept
        {
        }

        static decltype(auto) get_allocator()
        {
            return AllocProvider::get_allocator();
        }

        [[nodiscard]]
        constexpr T* allocate(size_t n)
        {
            void* const ptr = get_allocator().realloc(nullptr, sizeof(T) * n);
            return reinterpret_cast<T*>(ptr);
        }

        void deallocate(T* p, [[maybe_unused]] std::size_t n)
        {
            get_allocator().free(reinterpret_cast<void*>(p));
        }

        template <typename U, MemAllocatorProvider UProvider>
        bool operator==(const StatelessStdAllocator<U, UProvider>&) const
        {
            const IMemAllocator& a1 = StatelessStdAllocator<U, UProvider>::get_allocator();
            const IMemAllocator& a2 = get_allocator();

            return &a1 == &a2;
        }
    };

    // template <typename T, auto F>
    // using StatelessStdAllocatorF = StatelessStdAllocator<T, decltype(F)>;

#if 0
    /*
     */
    template <typename T>
    class MemAllocatorStdWrapper
    {
    public:
        using value_type = T;
        using propagate_on_container_move_assignment = std::true_type;

        MemAllocatorStdWrapper(IMemAllocator& allocator) noexcept :
            m_allocator(allocator)
        {
        }

        /// move is forbidden: because original ( \p other) allocator should always refer to valid allocator (will use it to free its own state)
        MemAllocatorStdWrapper(const MemAllocatorStdWrapper<T>& other) noexcept :
            m_allocator(other.m_allocator)
        {
        }

        template <typename U,
                  std::enable_if_t<!std::is_same_v<U, T>, int> = 0>
        MemAllocatorStdWrapper(const MemAllocatorStdWrapper<U>& other) noexcept :
            m_allocator(other.getMemAllocator())
        {
        }

        MemAllocatorStdWrapper<T>& operator=([[maybe_unused]] const MemAllocatorStdWrapper<T>& other) noexcept
        {
            MY_DEBUG_ASSERT(&m_allocator == &other.getMemAllocator());
            return *this;
        }

        template <typename U,
                  std::enable_if_t<!std::is_same_v<U, T>, int> = 0>
        MemAllocatorStdWrapper<T>& operator=([[maybe_unused]] const MemAllocatorStdWrapper<U>& other) noexcept
        {
            MY_DEBUG_ASSERT(&m_allocator == &other.getMemAllocator());
            return *this;
        }

        [[nodiscard]] constexpr T* allocate(size_t n)
        {
            void* const ptr = m_allocator.alloc(sizeof(T) * n);
            MY_DEBUG_ASSERT(reinterpret_cast<uintptr_t>(ptr) % alignof(T) == 0);

            return reinterpret_cast<T*>(ptr);
        }

        void deallocate(T* p, [[maybe_unused]] std::size_t n)
        {
            // const size_t size = sizeof(T) * n;
            m_allocator.free(p);
        }

        template <typename U>
        bool operator==(const MemAllocatorStdWrapper<U>& other) const
        {
            return &m_allocator == &other.getMemAllocator();
        }

        IMemAllocator& getMemAllocator() const
        {
            return (m_allocator);
        }

    private:
        IMemAllocator& m_allocator;
    };

#endif
    MY_KERNEL_EXPORT IAlignedMemAllocator& getSystemAllocator();

    inline IAlignedMemAllocator* getSystemAllocatorPtr()
    {
        return &getSystemAllocator();
    }

    // MY_KERNEL_EXPORT MemAllocatorPtr createSystemAllocator(bool threadSafe);

    //    MY_KERNEL_EXPORT IMemAllocator& getDefaultAllocator();

    namespace mem_detail
    {
        struct SystemAllocatorProvider
        {
            static IMemAllocator& get_allocator()
            {
                return getSystemAllocator();
            }
        };

    }  // namespace mem_detail

    template <typename T>
    using DefaultStdAllocator = StatelessStdAllocator<T, mem_detail::SystemAllocatorProvider>;

}  // namespace my
