#if 0
// runtime_stack.cpp
//
// Copyright (c) N-GINN LLC., 2023-2025. All rights reserved.
//
#include "runtime/memory/runtime_stack.h"

#include "runtime/memory/mem_page_allocator.h"
#include "runtime/rtti/rtti_impl.h"

namespace my
{
    namespace
    {
        static thread_local RuntimeStackGuard* s_currentStackGuard = nullptr;
    }

    class RuntimeStackAllocator final : public MemAllocator
    {
        MY_REFCOUNTED_CLASS(my::RuntimeStackAllocator, MemAllocator)

    public:
        RuntimeStackAllocator(const Kilobyte size) :
            m_pageAllocator(size.bytesCount())
        {
        }

        ~RuntimeStackAllocator()
        {
            // std::cout << "max rt size: " << m_allocator.allocatedSize() << std::endl;
        }

        void restore(size_t offset)
        {
            m_offset = offset;
        }

        size_t getOffset() const
        {
            return m_offset;
        }

    private:
        void* realloc(void* prevPtr, size_t size) override;

        void* allocAligned(size_t size, size_t alignment) override;

        void free(void* ptr) override;

        void freeAligned(void* ptr, std::optional<size_t> alignment) override;

        PageAllocator m_pageAllocator;
        size_t m_offset = 0;
    };

    void* RuntimeStackAllocator::realloc([[maybe_unused]] void* prevPtr, size_t size)
    {
        G_ASSERT(!prevPtr, "Re-allocation is not supported for runtime stack allocator");

        return this->allocAligned(size, alignof(std::max_align_t));
    }

    void* RuntimeStackAllocator::allocAligned(size_t size, size_t alignment)
    {
        std::byte* topPtr = reinterpret_cast<std::byte*>(m_pageAllocator.getBase()) + m_offset;

        // const size_t alignment = optionalAlignment ? *optionalAlignment : alignof(std::max_align_t);
        G_ASSERT(isPowerOf2(alignment), "Alignment must be power of two");

        const size_t alignedBlockSize = alignedSize(size, alignment);

        // padding = offset from topPtr to make result address aligned.
        const size_t d = reinterpret_cast<ptrdiff_t>(topPtr) % alignment;
        const size_t padding = d == 0 ? 0 : (alignment - d);

        const size_t allocationSize = alignedBlockSize + padding;

        if(const size_t newOffset = m_offset + allocationSize; newOffset <= m_pageAllocator.getAllocatedSize())
        {
            m_offset = newOffset;
        }
        else
        {
            topPtr = reinterpret_cast<std::byte*>(m_pageAllocator.alloc(allocationSize));
            G_ASSERT_ALWAYS(topPtr, "Runtime stack is out of memory");
            m_offset = m_pageAllocator.getAllocatedSize();
        }

        return topPtr + padding;
    }

    void RuntimeStackAllocator::free([[maybe_unused]] void* ptr)
    {  // TODO: need to check that ptr belongs to the current runtime stack allocation range.
    }

    void RuntimeStackAllocator::freeAligned([[maybe_unused]] void* ptr, [[maybe_unused]] std::optional<size_t> alignment)
    {  // TODO: need to check that ptr belongs to the current runtime stack allocation range.
    }

    RuntimeStackGuard::RuntimeStackGuard() :
        m_prev(std::exchange(s_currentStackGuard, this))
    {
        if(m_prev && m_prev->m_allocator)
        {
            m_allocator = m_prev->m_allocator;
            m_top = m_prev->m_allocator->as<RuntimeStackAllocator&>().getOffset();
        }
    }

    RuntimeStackGuard::RuntimeStackGuard(Kilobyte size) :
        m_prev(std::exchange(s_currentStackGuard, this)),
        m_allocator(rtti::createInstance<RuntimeStackAllocator, MemAllocator>(size))
    {
    }

    RuntimeStackGuard::~RuntimeStackGuard()
    {
        if(m_allocator)
        {
            m_allocator->as<RuntimeStackAllocator&>().restore(m_top);
        }

        G_ASSERT_ALWAYS(std::exchange(s_currentStackGuard, m_prev) == this);
    }

    const MemAllocatorPtr& RuntimeStackGuard::getAllocator()
    {
        const MemAllocatorPtr& allocator = (s_currentStackGuard && s_currentStackGuard->m_allocator) ? s_currentStackGuard->m_allocator : getCrtAllocator();
        G_ASSERT(allocator);

        return (allocator);
    }
}  // namespace my

#endif
