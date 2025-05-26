// #my_engine_source_file

#pragma once
#include "my/kernel/kernel_config.h"
#include "my/memory/mem_allocator.h"
#include "my/utils/preprocessor.h"


namespace my
{
    class RuntimeStackGuard
    {
    public:
        static IAlignedMemAllocator& get_allocator();

        RuntimeStackGuard();
        RuntimeStackGuard(Kilobyte size);
        ~RuntimeStackGuard();
        RuntimeStackGuard(const RuntimeStackGuard&) = delete;
        RuntimeStackGuard& operator=(const RuntimeStackGuard&) = delete;

    private:
        RuntimeStackGuard* const m_prev = nullptr;
        MemAllocatorPtr m_allocator;
        size_t m_top = 0;
    };

    struct MY_ABSTRACT_TYPE IStackAllocatorInfo
    {
        MY_TYPEID(my::IStackAllocatorInfo)

        virtual ~IStackAllocatorInfo() = default;

        virtual uintptr_t getAllocationOffset() const = 0;
    };

    inline IAlignedMemAllocator& get_rt_stack_allocator()
    {
        return RuntimeStackGuard::get_allocator();
    }

    inline IAlignedMemAllocator* get_rt_stack_allocator_ptr()
    {
        return &RuntimeStackGuard::get_allocator();
    }

    struct RtStackAllocatorProvider
    {
        static IAlignedMemAllocator& get_allocator()
        {
            return RuntimeStackGuard::get_allocator();
        }
    };

    template <typename T>
    using RtStackStdAllocator = StatelessStdAllocator<T, RtStackAllocatorProvider>;

    template <template <typename T, typename Allocator, typename...> class Container, typename T>
    using StackContainer = Container<T, RtStackStdAllocator<T>>;

}  // namespace my

// clang-format off
#define rtstack_init(initialSize)  const ::my::RuntimeStackGuard ANONYMOUS_VAR(rtStack__){initialSize}

#define rtstack_scope const ::my::RuntimeStackGuard ANONYMOUS_VAR(rtStack__){}

// clang-format on
