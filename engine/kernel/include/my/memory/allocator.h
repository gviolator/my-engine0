// #my_engine_source_file
#pragma once

#include "my/kernel/kernel_config.h"
//#include "my/memory/mem_base.h"
#include "my/rtti/ptr.h"
#include "my/rtti/rtti_object.h"

//#include <concepts>
#include <memory_resource>
#include <type_traits>
#include <limits>

// #include "my/utils/type_utility.h"

namespace my {
struct IAllocator;


struct MY_ABSTRACT_TYPE IAllocator : IRefCounted
{
    static inline constexpr size_t DefaultAlignment = alignof(std::max_align_t);
    static inline constexpr size_t UnspecifiedValue = std::numeric_limits<size_t>::max();

    MY_INTERFACE(my::IAllocator, IRefCounted)

    [[nodiscard]] virtual void* alloc(size_t size, size_t alignment = DefaultAlignment) = 0;

    [[nodiscard]] virtual void* realloc(void* oldPtr, size_t size, size_t alignment = UnspecifiedValue) = 0;

    virtual void free(void* ptr, size_t size = UnspecifiedValue, size_t alignment = UnspecifiedValue) = 0;

    virtual size_t getMaxAlignment() const = 0;

    virtual void setName(const char*)
    {
    }

    virtual const char* getName() const
    {
        return "";
    }

    virtual std::pmr::memory_resource* getMemoryResource() const;
};

using AllocatorPtr = my::Ptr<IAllocator>;

template <typename U>
concept IsMemAllocatorRef = std::is_reference_v<U> && std::is_base_of_v<IAllocator, std::remove_reference_t<U>>;

// std::is_assignable_v<IAllocator*, U*>;

template <typename T>
concept MemAllocatorProvider = requires {
    { T::getAllocator() } -> IsMemAllocatorRef;
};

/*
 *
 */
template <typename T, MemAllocatorProvider AllocProvider>
class StatelessStdAllocator
{
public:
    /// static_assert(MemAllocatorProvider<decltype(AllocProvider)>);
    using value_type = T;
    using propagate_on_container_move_assignment = std::true_type;

    StatelessStdAllocator() noexcept = default;

    template <typename U, MemAllocatorProvider UProvider>
    StatelessStdAllocator(const StatelessStdAllocator<U, UProvider>&) noexcept
    {
    }

    static decltype(auto) getAllocator()
    {
        return AllocProvider::getAllocator();
    }

    [[nodiscard]]
    constexpr T* allocate(size_t n)
    {
        void* const ptr = getAllocator().realloc(nullptr, sizeof(T) * n);
        return reinterpret_cast<T*>(ptr);
    }

    void deallocate(T* p, [[maybe_unused]] std::size_t n)
    {
        getAllocator().free(reinterpret_cast<void*>(p));
    }

    template <typename U, MemAllocatorProvider UProvider>
    bool operator==(const StatelessStdAllocator<U, UProvider>&) const
    {
        const IAllocator& a1 = StatelessStdAllocator<U, UProvider>::getAllocator();
        const IAllocator& a2 = getAllocator();

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

        MemAllocatorStdWrapper(IAllocator& allocator) noexcept :
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

        IAllocator& getMemAllocator() const
        {
            return (m_allocator);
        }

    private:
        IAllocator& m_allocator;
    };

#endif

MY_KERNEL_EXPORT AllocatorPtr createDefaultGenericAllocator(bool threadSafe = true);

MY_KERNEL_EXPORT IAllocator& getDefaultAllocator();

MY_KERNEL_EXPORT IAllocator& getDefaultAlignedAllocator();

inline IAllocator* getDefaultAllocatorPtr()
{
    return &getDefaultAllocator();
}

inline IAllocator* getDefaultAlignedAllocatorPtr()
{
    return &getDefaultAlignedAllocator();
}

}  // namespace my
