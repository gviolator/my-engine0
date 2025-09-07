// #my_engine_source_file
#pragma once
#include "my/memory/allocator.h"
#include "my/memory/internal/allocator_base.h"
#include "my/rtti/rtti_impl.h"

namespace my {

/**
*/
class CrtAllocator final : public AllocatorWithMemResource<CrtAllocator>
{
    MY_REFCOUNTED_CLASS(my::CrtAllocator, IAllocator)

public:
    void* alloc(size_t size, size_t alignment) override;
    void* realloc(void* oldPtr, size_t size, size_t alignment) override;
    void free(void* ptr, size_t size, size_t alignment) override;
    size_t getMaxAlignment() const override;
};

/**
*/
class AlignedCrtAllocator final : public AllocatorWithMemResource<AlignedCrtAllocator>
{
    MY_REFCOUNTED_CLASS(my::AlignedCrtAllocator, IAllocator)

public:
    void* alloc(size_t size, size_t alignment) override;
    void* realloc(void* oldPtr, size_t size, size_t alignment) override;
    void free(void* ptr, size_t size, size_t alignment) override;
    size_t getMaxAlignment() const override;
};

}  // namespace my
