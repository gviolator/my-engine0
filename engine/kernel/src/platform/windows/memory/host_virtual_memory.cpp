// #my_engine_source_file
#include "my/diag/check.h"
#include "my/memory/host_memory.h"
#include "my/threading/lock_guard.h"
#include "my/threading/mutex_no_lock.h"
#include "my/threading/spin_lock.h"

namespace my
{
    class WindowsHostVirtualMemory final : public IHostMemory
    {
    public:
        WindowsHostVirtualMemory(const WindowsHostVirtualMemory&) = delete;
        WindowsHostVirtualMemory& operator=(const WindowsHostVirtualMemory&) = delete;

        WindowsHostVirtualMemory(size_t size) :
            m_size(alignedSize(size, mem::AllocationGranularity))
        {
            m_basePtr = reinterpret_cast<std::byte*>(::VirtualAlloc(nullptr, static_cast<SIZE_T>(m_size), MEM_RESERVE, PAGE_READWRITE));
            MY_FATAL(m_basePtr);
        }

        ~WindowsHostVirtualMemory()
        {
            ::VirtualFree(m_basePtr, 0, MEM_FREE);
        }

    private:
        MemRegion allocPages(size_t size) override
        {
            size = alignedSize(size, mem::PageSize);
            size_t allocOffset = m_allocOffset;

            for (; !m_allocOffset.compare_exchange_strong(allocOffset, m_allocOffset + size, std::memory_order_relaxed);)
            {
            }

            const size_t requiredCommitedSize = allocOffset + size;

            if (m_commitedSize.load(std::memory_order_relaxed) < requiredCommitedSize)
            {
                lock_(m_mutex);

                const size_t currentCommitedSize = m_commitedSize.load(std::memory_order_relaxed);

                if (currentCommitedSize < requiredCommitedSize)
                {
                    const size_t commitSize = requiredCommitedSize - currentCommitedSize;
                    MY_DEBUG_CHECK(commitSize % mem::PageSize == 0);

                    const size_t newCommitedSize = currentCommitedSize + commitSize;
                    if (newCommitedSize > m_size)
                    {
                        return MemRegion{};
                    }

                    ::VirtualAlloc(m_basePtr + currentCommitedSize, static_cast<SIZE_T>(commitSize), MEM_COMMIT, PAGE_READWRITE);
                    m_commitedSize.store(newCommitedSize, std::memory_order_relaxed);
                }
            }

            return MemRegion{m_basePtr + allocOffset, size};
        }

        void freePages(MemRegion&&) override
        {
            MY_FAILURE("WindowsHostVirtualMemory::freePages not implemented");
        }

        constexpr Byte getPageSize() const override
        {
            return mem::PageSize;
        }

        const size_t m_size;
        std::atomic<size_t> m_commitedSize = 0;
        std::atomic<size_t> m_allocOffset = 0;
        std::byte* m_basePtr;

        std::mutex m_mutex;
    };

    HostMemoryPtr createHostVirtualMemory(Byte maxSize, [[maybe_unused]] bool threadSafe)
    {
        return std::make_shared<WindowsHostVirtualMemory>(maxSize);
    }

}  // namespace my
