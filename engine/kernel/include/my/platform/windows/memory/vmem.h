// runtime/memory/mem_page_allocator.h
//
// Copyright (c) N-GINN LLC., 2023-2025. All rights reserved.
//

#pragma once
#include "osApiWrappers/dag_virtualMem.h"
#include "runtime/memory/mem_base.h"

// // #define DISABLE_WIN_MEMORYAPI

// #if ENGINE_OS_WINDOWS_DESKTOP && !defined(DISABLE_WIN_MEMORYAPI)
// #define USE_WINAPI_VMEM_ALLOCATION
// #endif

// #ifdef USE_WINAPI_VMEM_ALLOCATION
// #include <memoryapi.h>
// #endif

namespace my
{
    class PageAllocator
    {
    public:
        PageAllocator(size_t size) :
            m_size(my::alignedSize(size, my::mem::AllocationGranularity))
        {
            m_basePtr = os_virtual_mem_alloc(m_size); 
        }

        ~PageAllocator()
        {
            os_virtual_mem_free(m_basePtr, m_size);
        }

        void* alloc(size_t size) noexcept
        {
            const size_t newAllocOffset = m_allocOffset + size;
            auto bytePtr = reinterpret_cast<std::byte*>(m_basePtr);

            if(newAllocOffset > m_commitedSize)
            {
                const size_t commitSize = alignedSize(newAllocOffset - m_commitedSize, my::mem::PageSize);
                const size_t newCommitedSize = m_commitedSize + commitSize;
                if(newCommitedSize > m_size)
                {
                    return nullptr;
                }

                os_virtual_mem_commit(bytePtr + m_commitedSize, commitSize);
                m_commitedSize = newCommitedSize;
            }

            void* const ptr = bytePtr + m_allocOffset;
            m_allocOffset = newAllocOffset;
            return ptr;
        }

        void* alloc(size_t size, size_t alignment) noexcept
        {
            G_ASSERT(isPowerOf2(alignment), "Alignment must be power of two");

            const size_t alignedBlockSize = alignedSize(size, alignment);

            // make result address aligned:
            const size_t d = reinterpret_cast<ptrdiff_t>(reinterpret_cast<std::byte*>(m_basePtr) + m_allocOffset) % alignment;
            const size_t padding = d == 0 ? 0 : (alignment - d);

            std::byte* const ptr = reinterpret_cast<std::byte*>(alloc(alignedBlockSize + padding));
            return ptr == nullptr ? nullptr : ptr + padding;
        }

        inline bool contains(const void* ptr) const
        {
            return m_basePtr <= ptr && ptr < (reinterpret_cast<std::byte*>(m_basePtr) + m_size);
        }

        inline void* getBase() const
        {
            return m_basePtr;
        }

        inline size_t getSize() const
        {
            return m_size;
        }

        inline size_t getAllocatedSize() const
        {
            return m_allocOffset;
        }

        inline size_t getCommitedSize() const
        {
            return m_commitedSize;
        }

    private:
        const size_t m_size;
        size_t m_commitedSize = 0;
        size_t m_allocOffset = 0;
        void* m_basePtr;
    };


    // #ifdef USE_WINAPI_VMEM_ALLOCATION
    //     using PageAllocator = ::Runtime::RtDetail::PageAllocator<::Runtime::RtDetail::WinApiMemoryDriver>;
    // #else
    //     using PageAllocator = ::Runtime::RtDetail::PageAllocator<::Runtime::RtDetail::GenericMemoryDriver>;
    // #endif

}  // namespace my

// #endif  // ENGINE_OS_EMSCRIPTEN
