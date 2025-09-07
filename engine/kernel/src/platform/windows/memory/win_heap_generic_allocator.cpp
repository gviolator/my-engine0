// #my_engine_source_file
#include "my/memory/allocator.h"
#include "my/memory/internal/allocator_base.h"
#include "my/platform/windows/diag/win_error.h"
#include "my/rtti/rtti_impl.h"

namespace my {

class WinHeapAllocator final : public AllocatorWithMemResource<WinHeapAllocator>
{
    MY_REFCOUNTED_CLASS(my::WinHeapAllocator, IAllocator)

public:
    WinHeapAllocator(bool threadSafe)
    {
        const DWORD opts = threadSafe ? 0 : HEAP_NO_SERIALIZE;
        const SIZE_T initialSize = mem::AllocationGranularity;

        m_heap = HeapCreate(opts, initialSize, 0);
        MY_FATAL(m_heap != NULL, diag::getWinErrorMessageA(diag::getAndResetLastErrorCode()));
    }

    ~WinHeapAllocator()
    {
        ::HeapDestroy(m_heap);
    }

    void* alloc(size_t size, [[maybe_unused]] size_t alignment) override
    {
        MY_DEBUG_FATAL(m_heap != NULL);
        MY_DEBUG_FATAL(checkAllocAlignment(alignment, MEMORY_ALLOCATION_ALIGNMENT));

        void* const ptr = ::HeapAlloc(m_heap, getAllocOpts(), size);
        MY_DEBUG_FATAL(reinterpret_cast<uintptr_t>(ptr) % alignment == 0);
        return ptr;
    }

    void* realloc(void* oldPtr, size_t size, size_t alignment) override
    {
        MY_DEBUG_FATAL(m_heap != NULL);
        MY_DEBUG_FATAL(checkAllocAlignment(alignment, MEMORY_ALLOCATION_ALIGNMENT));

        return ::HeapReAlloc(m_heap, getAllocOpts(), oldPtr, size);
    }

    void free(void* ptr, [[maybe_unused]] size_t size, size_t alignment) override
    {
        MY_DEBUG_FATAL(m_heap != NULL);
        MY_DEBUG_FATAL(checkAllocAlignment(alignment, MEMORY_ALLOCATION_ALIGNMENT));

        ::HeapFree(m_heap, 0, ptr);
    }

    size_t getMaxAlignment() const override
    {
        return MEMORY_ALLOCATION_ALIGNMENT;
    }

private:
    static inline constexpr size_t getAllocOpts()
    {
        constexpr DWORD opts =
#ifndef NDEBUG
            HEAP_ZERO_MEMORY;
#else
            0;
#endif
        return opts;
    }

    HANDLE m_heap = NULL;
};

AllocatorPtr createDefaultGenericAllocator(bool threadSafe)
{
    return rtti::createInstance<WinHeapAllocator>(threadSafe);
}

}  // namespace my
