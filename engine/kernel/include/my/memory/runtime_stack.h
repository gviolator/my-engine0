// #my_engine_source_file

#pragma once
#include "my/kernel/kernel_config.h"
#include "my/memory/allocator.h"
#include "my/memory/mem_base.h"
#include "my/utils/preprocessor.h"

namespace my {
class MY_KERNEL_EXPORT RuntimeStackGuard
{
public:
    static IAllocator& getAllocator();

    RuntimeStackGuard();
    RuntimeStackGuard(Kilobyte size);
    ~RuntimeStackGuard();
    RuntimeStackGuard(const RuntimeStackGuard&) = delete;
    RuntimeStackGuard& operator=(const RuntimeStackGuard&) = delete;
    uintptr_t getMyTop() const;

private:
    RuntimeStackGuard* const m_prev = nullptr;
    AllocatorPtr m_allocator;
    const uintptr_t m_top = 0;
#if MY_DEBUG_ASSERT_ENABLED

#endif
};

struct MY_ABSTRACT_TYPE IStackAllocatorInfo
{
    MY_TYPEID(my::IStackAllocatorInfo)

    virtual ~IStackAllocatorInfo() = default;

    virtual uintptr_t getAllocationOffset() const = 0;
};

inline IAllocator& GetRtStackAllocator()
{
    return RuntimeStackGuard::getAllocator();
}

inline IAllocator* GetRtStackAllocatorPtr()
{
    return &RuntimeStackGuard::getAllocator();
}

struct RtStackAllocatorProvider
{
    static IAllocator& getAllocator()
    {
        return RuntimeStackGuard::getAllocator();
    }
};

template <typename T>
using RtStackStdAllocator = StatelessStdAllocator<T, RtStackAllocatorProvider>;

template <template <typename T, typename Allocator, typename...> class Container, typename T>
using StackContainer = Container<T, RtStackStdAllocator<T>>;

}  // namespace my

// clang-format off
#define rtstack_init(initialSize)  const ::my::RuntimeStackGuard ANONYMOUS_VAR(rtStack__){initialSize}

#define rtstack_scope const ::my::RuntimeStackGuard ANONYMOUS_VAR(rtStack__);

// clang-format on
