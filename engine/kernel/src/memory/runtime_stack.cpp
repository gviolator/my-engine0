// #my_engine_source_file
#include "my/memory/runtime_stack.h"

#include "my/diag/assert.h"
#include "my/memory/host_memory.h"
#include "my/rtti/rtti_impl.h"


namespace my
{
    namespace
    {
        static thread_local RuntimeStackGuard* s_currentStackGuard = nullptr;
    }

    class RuntimeStackAllocator final : public IAlignedMemAllocator, public IStackAllocatorInfo
    {
        MY_REFCOUNTED_CLASS(my::RuntimeStackAllocator, IAlignedMemAllocator, IStackAllocatorInfo)

    public:
        static constexpr size_t MaxAlignment = 128;
        static constexpr size_t DefaultAlignment = alignof(std::max_align_t);

        RuntimeStackAllocator(const Kilobyte size) :
            m_pageAllocator(createHostVirtualMemory(size, false))
        {
        }

        ~RuntimeStackAllocator()
        {
            // std::cout << "max rt size: " << m_allocator.allocatedSize() << std::endl;
        }

        void restore(size_t offset)
        {
            m_allocOffset = offset;
        }

        size_t getOffset() const
        {
            return m_allocOffset;
        }

        void* alloc(size_t size) override
        {
            return reallocAligned(nullptr, size, DefaultAlignment);
        }

        void* realloc(void* oldPtr, size_t size) override
        {
            return reallocAligned(oldPtr, size, DefaultAlignment);
        }

        void free(void* ptr) override
        {
            freeAligned(ptr, DefaultAlignment);
        }

        size_t getAllocationAlignment() const override
        {
            return MaxAlignment;
        }

        void* allocAligned(size_t size, size_t alignment) override
        {
            return reallocAligned(nullptr, size, alignment);
        }

        void* reallocAligned(void* oldPtr, size_t size, size_t alignment) override;

        void freeAligned(void* ptr, size_t alignment) override;

        uintptr_t getAllocationOffset() const override
        {
            return m_allocOffset;
        }

    private:
        HostMemoryPtr m_pageAllocator;
        IHostMemory::MemRegion m_allocatedRegion;
        uintptr_t m_allocOffset = 0;
    };

    void* RuntimeStackAllocator::reallocAligned([[maybe_unused]] void* oldPtr, const size_t size, const size_t alignment)
    {
        MY_DEBUG_ASSERT(oldPtr == nullptr, "Reallocation for stack allocator is not implemented");
        MY_DEBUG_ASSERT(is_power_of2(alignment), "Alignment must be power of two");
        MY_DEBUG_ASSERT(alignment <= MaxAlignment, "Requested alignment ({}) exceed max alignment ({})", alignment, MaxAlignment);

        const size_t d = m_allocOffset % alignment;
        const size_t padding = d == 0 ? 0 : (alignment - d);
        const size_t allocationSize = aligned_size(size, alignment) + padding;
        const size_t newAllocOffset = m_allocOffset + allocationSize;

        if (!m_allocatedRegion)
        {
            m_allocatedRegion = m_pageAllocator->allocPages(allocationSize);
            MY_FATAL(m_allocatedRegion);
        }
        else if (m_allocatedRegion.size() < newAllocOffset)
        {
            IHostMemory::MemRegion nextRegion = m_pageAllocator->allocPages(allocationSize);
            MY_FATAL(nextRegion);
            MY_FATAL(IHostMemory::MemRegion::is_adjacent(m_allocatedRegion, nextRegion));

            m_allocatedRegion += std::move(nextRegion);
        }

        void* const newPtr = reinterpret_cast<std::byte*>(m_allocatedRegion.basePtr()) + m_allocOffset + padding;
        m_allocOffset = newAllocOffset;

        return newPtr;
    }

    void RuntimeStackAllocator::freeAligned(void* ptr, [[maybe_unused]] size_t alignment)
    {  // TODO: need to check that ptr belongs to the current runtime stack allocation range.
        MY_DEBUG_ASSERT(is_power_of2(alignment), "Invalid alignment");
        MY_DEBUG_ASSERT(alignment <= MaxAlignment, "Invalid alignment");
        if (!ptr)
        {
            return;
        }

        [[maybe_unused]] const auto offset = reinterpret_cast<uintptr_t>(ptr);
        [[maybe_unused]] const auto base = reinterpret_cast<uintptr_t>(m_allocatedRegion.basePtr());

        MY_DEBUG_ASSERT(base <= offset && offset < base + m_allocatedRegion.size());
    }

    RuntimeStackGuard::RuntimeStackGuard() :
        m_prev(std::exchange(s_currentStackGuard, this))
    {
        if (m_prev && m_prev->m_allocator)
        {
            m_allocator = m_prev->m_allocator;
            m_top = m_prev->m_allocator->as<RuntimeStackAllocator&>().getOffset();
        }
    }

    RuntimeStackGuard::RuntimeStackGuard(Kilobyte size) :
        m_prev(std::exchange(s_currentStackGuard, this)),
        m_allocator(rtti::createInstance<RuntimeStackAllocator, IMemAllocator>(size))
    {
    }

    RuntimeStackGuard::~RuntimeStackGuard()
    {
        if (m_allocator)
        {
            m_allocator->as<RuntimeStackAllocator&>().restore(m_top);
        }

        MY_ASSERT(std::exchange(s_currentStackGuard, m_prev) == this);
    }

    IAlignedMemAllocator& RuntimeStackGuard::get_allocator()
    {
        if (s_currentStackGuard && s_currentStackGuard->m_allocator)
        {
            return s_currentStackGuard->m_allocator->as<IAlignedMemAllocator&>();
        }

        IAlignedMemAllocator& allocator = getSystemAllocator();
        return allocator.as<IAlignedMemAllocator&>();
    }
}  // namespace my
