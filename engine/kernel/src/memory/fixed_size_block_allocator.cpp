// #my_engine_source_file

#include "my/memory/fixed_size_block_allocator.h"
#include "my/memory/host_memory.h"
#include "my/memory/internal/allocator_base.h"
#include "my/rtti/rtti_impl.h"
#include "my/threading/lock_guard.h"
#include "my/threading/mutex_no_lock.h"
#include "my/threading/spin_lock.h"

namespace my {

/**
 *
 */
class Pool
{
public:
    static constexpr size_t BlockAlignment = 16;

    Pool(IHostMemory& mem, size_t blockSize) :
        m_memory(mem),
        m_blockSize(blockSize)
    {
        constexpr size_t PreAllocateBlockCount = 10;

        m_pages.emplace_back(m_memory.allocPages(blockSize * PreAllocateBlockCount));
    }

    size_t getBlockSize() const
    {
        return m_blockSize;
    }

    void* allocate()
    {
        void* ptr = nullptr;

        if (!m_freeBlocksBegin)
        {
            ptr = allocateNewBlock();
        }
        else
        {
            Block* const block = reinterpret_cast<Block*>(m_freeBlocksBegin);
            m_freeBlocksBegin = block->next;
            ptr = block;
        }

#ifndef NDEBUG
        if (ptr)
        {
            memset(ptr, 0, m_blockSize);
        }
#endif

        return ptr;
    }

    void free(void* ptr)
    {
        reinterpret_cast<Block*>(ptr)->next = m_freeBlocksBegin;
        m_freeBlocksBegin = ptr;
    }

private:
    struct MemRegionEntry
    {
        IHostMemory::MemRegion pages;
        size_t offset = 0;
    };

    struct Block
    {
        void* next;
    };

    void* allocateNewBlock()
    {
        MY_DEBUG_ASSERT(!m_pages.empty());

        MemRegionEntry* region = &m_pages.back();
        if (const size_t availSize = region->pages.size() - region->offset; availSize < m_blockSize)
        {
            region = &m_pages.emplace_back(m_memory.allocPages(m_blockSize));
        }

        MY_DEBUG_FATAL(region->pages.size() >= m_blockSize);

        const size_t allocOffset = std::exchange(region->offset, region->offset + m_blockSize);
        return reinterpret_cast<std::byte*>(region->pages.basePtr()) + allocOffset;
    }

    std::vector<MemRegionEntry> m_pages;
    IHostMemory& m_memory;
    const size_t m_blockSize;
    void* m_freeBlocksBegin = nullptr;
};

/**
 *
 */
template <typename Mutex>
class FixedSizeBlockAllocator final : public AllocatorWithMemResource<FixedSizeBlockAllocator<Mutex>>
{
public:
    MY_REFCOUNTED_CLASS(FixedSizeBlockAllocator, IAllocator)

    FixedSizeBlockAllocator(HostMemoryPtr memory, size_t blockSize) :
        m_memory(memory),
        m_pool{*memory, blockSize}
    {
    }

    // ~FixedSizeBlockAllocator()
    // {
    //     // #ifndef NDEBUG
    //     //             std::stringstream ss;
    //     //             ss << std::format("[Allocator]\nClear pool allocator, total pools: ({})\n", m_pools.size());

    //     //             for(auto& pool : m_pools)
    //     //             {
    //     //                 ss << std::format("* [{0}], size = {1} bytes", pool.getBlockSize(), pool.getSize());
    //     //             }
    //     //             //LOG_DEBUG(ss.str());
    //     // #endif
    // }

    void* alloc(size_t size, [[maybe_unused]] size_t align) override
    {
        MY_DEBUG_FATAL(size <= m_pool.getBlockSize());
        MY_DEBUG_FATAL(checkAllocAlignment(align,Pool::BlockAlignment), "Invalid alignment ({})", align);

        lock_(m_mutex);

        void* const ptr = m_pool.allocate();
        MY_DEBUG_ASSERT(reinterpret_cast<uintptr_t>(ptr) % Pool::BlockAlignment == 0);

        return ptr;
    }

    void* realloc(void* oldPtr, size_t size, [[maybe_unused]] size_t align) override
    {
        MY_DEBUG_FATAL(size <= m_pool.getBlockSize());
        MY_DEBUG_FATAL(checkAllocAlignment(align,Pool::BlockAlignment), "Invalid alignment ({})", align);

        if (oldPtr)
        {
            //MY_DEBUG_FATAL(m_pool
            return oldPtr;
        }

        lock_(m_mutex);
        void* const ptr = m_pool.allocate();
        MY_DEBUG_ASSERT(reinterpret_cast<uintptr_t>(ptr) % Pool::BlockAlignment == 0);

        return ptr;
    }

    size_t getMaxAlignment() const override
    {
        return Pool::BlockAlignment;
    }

    // void* reallocAligned(void* oldPtr, size_t size, [[maybe_unused]] size_t alignment) override
    // {
    //     MY_DEBUG_FATAL(isPowerOf2(alignment));

    //     void* const newPtr = this->realloc(oldPtr, size);

    //     MY_DEBUG_FATAL(newPtr == nullptr || reinterpret_cast<uintptr_t>(newPtr) % alignment == 0);

    //     return newPtr;
    // }

    void free(void* ptr, size_t size, [[maybe_unused]] size_t align) override
    {
        MY_DEBUG_FATAL(size == IAllocator::UnspecifiedValue || size <= m_pool.getBlockSize());
        MY_DEBUG_FATAL(checkAllocAlignment(align, Pool::BlockAlignment));

        if (ptr)
        {
            lock_(m_mutex);
            m_pool.free(ptr);
        }
    }

    // void freeAligned(void* ptr, size_t) override
    // {
    //     free(ptr);
    // }

private:
    HostMemoryPtr m_memory;
    Pool m_pool;
    Mutex m_mutex;
};

AllocatorPtr createFixedSizeBlockAllocator(HostMemoryPtr hostMemory, Byte blockSize, bool threadSafe)
{
    const size_t alignedBlockSize = alignedSize(blockSize, Pool::BlockAlignment);

    using ThreadSafeAllocator = FixedSizeBlockAllocator<threading::SpinLock>;
    using ThreadUnsafeAllocator = FixedSizeBlockAllocator<threading::NoLockMutex>;

    if (threadSafe)
    {
        return rtti::createInstance<ThreadSafeAllocator>(std::move(hostMemory), alignedBlockSize);
    }

    return rtti::createInstance<ThreadUnsafeAllocator>(std::move(hostMemory), alignedBlockSize);
}

}  // namespace my
