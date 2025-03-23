// #my_engine_source_header
#include "my/memory/host_memory.h"

#include "my/diag/check.h"

#include <cstdlib>

namespace my
{
    IHostMemory::MemRegion::~MemRegion()
    {
    }

    IHostMemory::MemRegion::MemRegion(void* ptr, Byte size) :
        m_basePtr(ptr),
        m_size(size)
    {
        MY_DEBUG_CHECK(m_basePtr == nullptr || m_size > 0);
    }

    IHostMemory::MemRegion::MemRegion(MemRegion&& other) :
        m_basePtr(std::exchange(other.m_basePtr, nullptr)),
        m_size(std::exchange(other.m_size, 0))
    {
    }

    IHostMemory::MemRegion& IHostMemory::MemRegion::operator=(MemRegion&& other)
    {
        m_basePtr = std::exchange(other.m_basePtr, nullptr);
        m_size = std::exchange(other.m_size, 0);

        return *this;
    }

    std::strong_ordering IHostMemory::MemRegion::operator<=>(const MemRegion& other) const noexcept
    {
        MY_DEBUG_CHECK(m_basePtr != other.m_basePtr || m_size == other.m_size);
        return reinterpret_cast<uintptr_t>(m_basePtr) <=> reinterpret_cast<uintptr_t>(other.m_basePtr);
    }

    bool IHostMemory::MemRegion::operator==(const MemRegion& other) const noexcept
    {
        MY_DEBUG_CHECK(m_basePtr != other.m_basePtr || m_size == other.m_size);
        return m_basePtr == other.m_basePtr;
    }

    bool IHostMemory::MemRegion::isAdjacent(const MemRegion& left, const MemRegion& right)
    {
        if (!left || !right)
        {
            return false;
        }

        const auto res = left <=> right;

        if (res == std::strong_ordering::greater)
        {
            return reinterpret_cast<const std::byte*>(right.m_basePtr) + right.m_size == reinterpret_cast<const std::byte*>(left.m_basePtr);
        }
        else if (res == std::strong_ordering::less)
        {
            return reinterpret_cast<const std::byte*>(left.m_basePtr) + left.m_size == reinterpret_cast<const std::byte*>(right.m_basePtr);
        }

        return false;
    }

    class HostCrtMemory final : public IHostMemory
    {
        static constexpr size_t GuaranteedBlockAlignment = 128;

    public:
        HostCrtMemory() = default;

        ~HostCrtMemory()
        {}

    private:
        MemRegion allocPages(size_t size) override
        {
            size = alignedSize(size, mem::PageSize);

            void* const ptr = ::_aligned_malloc(size, GuaranteedBlockAlignment);
            MY_DEBUG_CHECK(reinterpret_cast<uintptr_t>(ptr) % GuaranteedBlockAlignment == 0);

            return MemRegion{ptr, size};
        }

        void freePages(MemRegion&& pages) override
        {
            ::_aligned_free(pages.getBasePtr());
        }

        Byte getPageSize() const override
        {
            return mem::PageSize;
        }
    };

    HostMemoryPtr createHostCrtMemory([[maybe_unused]] Byte maxSize)
    {
        return std::make_shared<HostCrtMemory>();
    }

}  // namespace my
