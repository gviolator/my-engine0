// #my_engine_source_header
#pragma once
#include <compare>
#include <memory>
#include <type_traits>

#include "my/kernel/kernel_config.h"
#include "my/memory/mem_base.h"
#include "my/rtti/type_info.h"

namespace my
{
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
            

            void* getBasePtr() const
            {
                return m_basePtr;
            }

            size_t getSize() const
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
    };

    ///**
    // */
    // struct MY_ABSTRACT_TYPE IHostContinuousMemory : IHostMemory
    //{
    //    MY_INTERFACE(my::IHostMemory, IRefCounted)

    //    virtual void* getBaseAddress() const = 0;

    //    virtual size_t getBaseSize() const = 0;

    //    virtual size_t getAllocatedSize() = 0;

    //    virtual bool contains(const void* address) const = 0;
    //};

    using HostMemoryPtr = std::shared_ptr<IHostMemory>;
    // using HostContinuousMemoryPtr = my::Ptr<IHostContinuousMemory>;

    MY_KERNEL_EXPORT HostMemoryPtr createHostVirtualMemory(Byte maxSize, bool threadSafe);

    MY_KERNEL_EXPORT HostMemoryPtr createHostCrtMemory(Byte maxSize = 0);

}  // namespace my
