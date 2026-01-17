// #my_engine_source_file
#pragma once

#include "my/diag/assert.h"
#include "my/memory/allocator.h"
#include "my/memory/mem_base.h"

#include <memory_resource>


namespace my {

/**
 */
template <typename Allocator>
class AllocatorMemoryResource final : public std::pmr::memory_resource
{
public:
    AllocatorMemoryResource(Allocator& allocator) :
        m_allocator{allocator}
    {
        static_assert(std::is_base_of_v<IAllocator, Allocator>);
    }

    AllocatorMemoryResource(const AllocatorMemoryResource&) = delete;
    AllocatorMemoryResource& operator=(const AllocatorMemoryResource&) = delete;

    void* do_allocate(size_t size, size_t align) override
    {
        MY_DEBUG_FATAL(align > 0 && isPowerOf2(align));
        MY_DEBUG_FATAL(align <= m_allocator.getMaxAlignment());

        void* ptr = m_allocator.alloc(size, align);

        MY_DEBUG_FATAL(ptr == nullptr || reinterpret_cast<uintptr_t>(ptr) % align == 0);
        return ptr;
    }

    void do_deallocate(void* ptr, size_t size, size_t align) override
    {
        m_allocator.free(ptr, size, align);
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
    {
        return static_cast<const std::pmr::memory_resource*>(this) == &other;
    }

private:
    Allocator& m_allocator;
};

template <typename T>
class AllocatorWithMemResource : public IAllocator
{

public:
    std::pmr::memory_resource* GetMemoryResource() const final
    {
        return &m_memResource;
    }

protected:
    AllocatorWithMemResource() :
        m_memResource{static_cast<T&>(*this)}
    {
    }

private:
    mutable AllocatorMemoryResource<T> m_memResource;
};

constexpr inline bool checkAllocAlignment(size_t align, size_t maxAlign)
{
    return align == IAllocator::UnspecifiedValue || (align <= maxAlign && isPowerOf2(align));
}

}  // namespace my