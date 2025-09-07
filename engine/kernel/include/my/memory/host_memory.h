// #my_engine_source_file
#pragma once
#include <compare>
#include <memory>

#include "my/kernel/kernel_config.h"
#include "my/memory/mem_base.h"
#include "my/rtti/type_info.h"

namespace my {
/**
 */
struct MY_ABSTRACT_TYPE IHostMemory
{
    class MY_KERNEL_EXPORT MemRegion
    {
    public:
        static bool isAdjacent(const MemRegion& left, const MemRegion& right);

        ~MemRegion();
        MemRegion() = default;
        MemRegion(void* ptr, Byte size);
        MemRegion(const MemRegion&) = delete;
        MemRegion(MemRegion&&);

        MemRegion& operator=(const MemRegion&) = delete;
        MemRegion& operator=(MemRegion&&);

        explicit operator bool() const
        {
            return m_basePtr != nullptr;
        }

        std::strong_ordering operator<=>(const MemRegion&) const noexcept;
        bool operator==(const MemRegion&) const noexcept;
        MemRegion& operator+=(MemRegion&& right) noexcept;

        void* basePtr() const
        {
            return m_basePtr;
        }

        size_t size() const
        {
            return m_size;
        }

    private:
        void* m_basePtr = nullptr;
        size_t m_size = 0;
    };

    virtual ~IHostMemory() = default;

    /**
     */
    virtual MemRegion allocPages(size_t size) = 0;

    /**
     */
    virtual void freePages(MemRegion&& pages) = 0;

    /**
     */
    virtual Byte getPageSize() const = 0;

    /**
        @returns Allocation granularity or zero if granularity is not meaningful for that allocator
    */
    virtual Byte getAllocationGranularity() const = 0;
};


using HostMemoryPtr = std::shared_ptr<IHostMemory>;

MY_KERNEL_EXPORT HostMemoryPtr createHostVirtualMemory(Byte maxSize, bool threadSafe);

MY_KERNEL_EXPORT HostMemoryPtr createCrtHostMemory(bool threadSafe = true);

}  // namespace my
