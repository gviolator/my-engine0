// #my_engine_source_file
#include "my/diag/assert.h"
#include "my/memory/host_memory.h"
#include "my/memory/internal/allocator_base.h"
#include "my/memory/runtime_stack.h"
#include "my/rtti/rtti_impl.h"

#include <compare>

// #include <unordered_set>

namespace my {
namespace {

static thread_local RuntimeStackGuard* s_currentStackGuard = nullptr;

#if MY_DEBUG_ASSERT_ENABLED
struct AllocationEntry
{
    uintptr_t marker = 0;
    uintptr_t mem = 0;

    AllocationEntry() = default;
    AllocationEntry(void* ptr) :
        marker(s_currentStackGuard->getMyTop()),
        mem(reinterpret_cast<uintptr_t>(ptr))
    {
    }

    std::strong_ordering operator<=>(const AllocationEntry& other) const noexcept
    {
        // MY_DEBUG_ASSERT(other.mem != mem || other.marker == marker);
        return mem <=> other.mem;
    }
};

static thread_local std::set<AllocationEntry> s_trackedAllocations;

inline void notifyAllocation(void* mem)
{
    if (s_currentStackGuard != nullptr)
    {
        const auto [iter, emplaceOk] = s_trackedAllocations.emplace(mem);
        MY_DEBUG_ASSERT(emplaceOk, "Stack allocation duplication");
    }
}

inline void notifyFree(void* mem)
{
    const auto count = s_trackedAllocations.erase(AllocationEntry{mem});
    MY_DEBUG_ASSERT(count == 1);
}

inline void checkNonFreedPointers(uintptr_t marker)
{
    const size_t nonFreedPointersCount = std::count_if(s_trackedAllocations.begin(), s_trackedAllocations.end(), [marker](const AllocationEntry& entry)
    {
        return entry.marker == marker;
    });

    MY_DEBUG_ASSERT(nonFreedPointersCount == 0, "There is ({}) non freed allocations", nonFreedPointersCount);
}

// void
#else
inline void notifyAllocation(void*)
{
}
inline void notifyFree(void*)
{
}
inline void checkNonFreedPointers(uintptr_t)
{
}

#endif
}  // namespace

class RuntimeStackAllocator final : public AllocatorWithMemResource<RuntimeStackAllocator>,
                                    public IStackAllocatorInfo
{
    MY_REFCOUNTED_CLASS(my::RuntimeStackAllocator, IAllocator, IStackAllocatorInfo)

public:
    using Base = AllocatorWithMemResource<RuntimeStackAllocator>;
    static constexpr size_t MaxAlignment = 16;

    RuntimeStackAllocator(const Kilobyte size) :
        Base(),
        m_pageAllocator(createHostVirtualMemory(size, false))
    {
    }

    ~RuntimeStackAllocator()
    {
        // std::cout << "max rt size: " << m_allocator.allocatedSize() << std::endl;
    }

    void restore(uintptr_t offset)
    {
#ifdef _DEBUG
        MY_DEBUG_ASSERT(m_allocOffset >= offset);
        if (const uintptr_t size = m_allocOffset - offset; size > 0)
        {
            void* const ptr = reinterpret_cast<std::byte*>(m_allocatedRegion.basePtr()) + offset;
            memset(ptr, 0, size);
        }
#endif
        m_allocOffset = offset;
    }

    size_t getOffset() const
    {
        return m_allocOffset;
    }

    void* alloc(size_t size, size_t align) override;
    // {
    //     return reallocAligned(nullptr, size, DefaultAlignment);
    // }

    void* realloc(void* oldPtr, size_t size, size_t align) override;
    // {
    //     return reallocAligned(oldPtr, size, DefaultAlignment);
    // }

    void free(void* ptr, size_t size, size_t align) override;
    // {
    //     freeAligned(ptr, DefaultAlignment);
    // }

    size_t getMaxAlignment() const override
    {
        return MaxAlignment;
    }

    // void* allocAligned(size_t size, size_t alignment) override
    // {
    //     return reallocAligned(nullptr, size, alignment);
    // }

    // void* reallocAligned(void* oldPtr, size_t size, size_t alignment) override;

    // void freeAligned(void* ptr, size_t alignment) override;

    uintptr_t getAllocationOffset() const override
    {
        return m_allocOffset;
    }

private:
    HostMemoryPtr m_pageAllocator;
    IHostMemory::MemRegion m_allocatedRegion;
    uintptr_t m_allocOffset = 0;
};

void* RuntimeStackAllocator::alloc(const size_t size, [[maybe_unused]] const size_t targetAlignment)
{
    MY_DEBUG_ASSERT(checkAllocAlignment(targetAlignment, MaxAlignment), "Invalid alignment ({}), max alignment ({})", targetAlignment, MaxAlignment);

    constexpr size_t alignment = MaxAlignment;
    const size_t allocationSize = alignedSize(size, alignment);
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
        MY_FATAL(IHostMemory::MemRegion::isAdjacent(m_allocatedRegion, nextRegion));

        m_allocatedRegion += std::move(nextRegion);
    }

    void* const newPtr = reinterpret_cast<std::byte*>(m_allocatedRegion.basePtr()) + std::exchange(m_allocOffset, newAllocOffset);

    MY_DEBUG_FATAL(reinterpret_cast<uintptr_t>(newPtr) % alignment == 0);
    MY_DEBUG_FATAL(m_allocOffset % MaxAlignment == 0);

    notifyAllocation(newPtr);
    return newPtr;
}

void* RuntimeStackAllocator::realloc([[maybe_unused]] void* oldPtr, const size_t size, [[maybe_unused]] const size_t alignment)
{
    // can not realloc, because don't known allocation actual size
    MY_DEBUG_ASSERT(oldPtr == nullptr, "Reallocation for stack allocator is not implemented");
    return alloc(size, alignment);

    // constexpr size_t

    // MY_DEBUG_ASSERT(oldPtr == nullptr, "Reallocation for stack allocator is not implemented");
    // MY_DEBUG_ASSERT(isPowerOf2(alignment), "Alignment must be power of two");
    // MY_DEBUG_ASSERT(alignment <= MaxAlignment, "Requested alignment ({}) exceed max alignment ({})", alignment, MaxAlignment);

    // const size_t d = m_allocOffset % alignment;
    // const size_t padding = d == 0 ? 0 : (alignment - d);
    // const size_t allocationSize = alignedSize(size, alignment) + padding;
    // const size_t newAllocOffset = m_allocOffset + allocationSize;

    // if (!m_allocatedRegion)
    // {
    //     m_allocatedRegion = m_pageAllocator->allocPages(allocationSize);
    //     MY_FATAL(m_allocatedRegion);
    // }
    // else if (m_allocatedRegion.size() < newAllocOffset)
    // {
    //     IHostMemory::MemRegion nextRegion = m_pageAllocator->allocPages(allocationSize);
    //     MY_FATAL(nextRegion);
    //     MY_FATAL(IHostMemory::MemRegion::isAdjacent(m_allocatedRegion, nextRegion));

    //     m_allocatedRegion += std::move(nextRegion);
    // }

    // void* const newPtr = reinterpret_cast<std::byte*>(m_allocatedRegion.basePtr()) + m_allocOffset + padding;
    // m_allocOffset = newAllocOffset;

    // notifyAllocation(newPtr);
    // return newPtr;
}

void RuntimeStackAllocator::free(void* ptr, [[maybe_unused]] size_t size, [[maybe_unused]] size_t alignment)
{
    MY_DEBUG_ASSERT(checkAllocAlignment(alignment, MaxAlignment), "Invalid alignment");
    if (ptr)
    {
        [[maybe_unused]] const auto offset = reinterpret_cast<uintptr_t>(ptr);
        [[maybe_unused]] const auto base = reinterpret_cast<uintptr_t>(m_allocatedRegion.basePtr());

        MY_DEBUG_ASSERT(base <= offset && offset < base + m_allocatedRegion.size());
        notifyFree(ptr);
    }
}

RuntimeStackGuard::RuntimeStackGuard() :
    m_prev(std::exchange(s_currentStackGuard, this)),
    m_top(m_prev && m_prev->m_allocator ? m_prev->m_allocator->as<const RuntimeStackAllocator&>().getOffset() : 0)
{
    if (m_prev)
    {
        m_allocator = m_prev->m_allocator;
    }
}

RuntimeStackGuard::RuntimeStackGuard(Kilobyte size) :
    m_prev(std::exchange(s_currentStackGuard, this)),
    m_allocator(rtti::createInstance<RuntimeStackAllocator, IAllocator>(size)),
    m_top(m_allocator->as<const RuntimeStackAllocator&>().getOffset())
{
}

RuntimeStackGuard::~RuntimeStackGuard()
{
#if MY_DEBUG_ASSERT_ENABLED
    checkNonFreedPointers(m_top);
#endif
    if (m_allocator)
    {
        m_allocator->as<RuntimeStackAllocator&>().restore(m_top);
    }

    MY_ASSERT(std::exchange(s_currentStackGuard, m_prev) == this);
}

uintptr_t RuntimeStackGuard::getMyTop() const
{
    return m_top;
}

IAllocator& RuntimeStackGuard::getAllocator()
{
    if (s_currentStackGuard && s_currentStackGuard->m_allocator)
    {
        return *s_currentStackGuard->m_allocator;
    }

    return getDefaultAllocator();
}
}  // namespace my
