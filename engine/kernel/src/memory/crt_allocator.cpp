// #my_engine_source_file
#include "crt_allocator.h"

namespace my {

void* CrtAllocator::alloc(size_t size, [[maybe_unused]] size_t alignment)
{
    static_assert(DefaultAlignment <= alignof(std::max_align_t));

    MY_DEBUG_FATAL(alignment != 0 && isPowerOf2(alignment) && alignment <= DefaultAlignment);

    void* const ptr = ::malloc(size);
    MY_DEBUG_FATAL(reinterpret_cast<uintptr_t>(ptr) % alignment == 0);

    return ptr;
}

void* CrtAllocator::realloc(void* oldPtr, size_t size, [[maybe_unused]] size_t alignment)
{
    MY_DEBUG_FATAL(checkAllocAlignment(alignment, DefaultAlignment));
    return ::realloc(oldPtr, size);
}

void CrtAllocator::free(void* ptr, [[maybe_unused]] size_t size, [[maybe_unused]] size_t alignment)
{
    MY_DEBUG_FATAL(checkAllocAlignment(alignment, DefaultAlignment));
    ::free(ptr);
}

size_t CrtAllocator::getMaxAlignment() const
{
    return DefaultAlignment;
}

void* AlignedCrtAllocator::alloc(size_t size, size_t alignment)
{
    MY_DEBUG_FATAL(alignment != 0 && isPowerOf2(alignment));

#ifdef _WIN32
    return ::_aligned_malloc(size, alignment);
#else
    return std::aligned_alloc(alignment, alignedSize(size, alignment));
#endif
}

void* AlignedCrtAllocator::realloc(void* oldPtr, size_t size, [[maybe_unused]] size_t alignment)
{
#ifdef _WIN32
    MY_DEBUG_FATAL(alignment != UnspecifiedValue, "Alignment MUST BE explicitly specified");
    MY_DEBUG_FATAL(alignment != 0 && isPowerOf2(alignment));
    auto const ptr = ::_aligned_realloc(oldPtr, size, alignment);
    return ptr;
#else
    return ::realloc(oldPtr, size);
#endif
}

void AlignedCrtAllocator::free(void* ptr, [[maybe_unused]] size_t size, [[maybe_unused]] size_t alignment)
{
#ifdef _WIN32
    ::_aligned_free(ptr);
#else
    ::free(ptr);
#endif
}

size_t AlignedCrtAllocator::getMaxAlignment() const
{
    return 32;
}

}  // namespace my
