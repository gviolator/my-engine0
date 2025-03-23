// runtime/memory/runtime_stack.h
//
// Copyright (c) N-GINN LLC., 2023-2025. All rights reserved.
//

#pragma once
#include "runtime/memory/mem_allocator.h"
#include "runtime/runtime_config.h"
#include "runtime/utils/preprocessor.h"

namespace my
{
    class RuntimeStackGuard
    {
    public:
        static const IMemAllocator::Ptr& getAllocator();

        RuntimeStackGuard();
        RuntimeStackGuard(Kilobyte size);
        ~RuntimeStackGuard();
        RuntimeStackGuard(const RuntimeStackGuard&) = delete;
        RuntimeStackGuard& operator=(const RuntimeStackGuard&) = delete;

    private:
        RuntimeStackGuard* const m_prev = nullptr;
        IMemAllocator::Ptr m_allocator;
        size_t m_top = 0;
    };

    struct RuntimeStackAllocatorProvider
    {
        static inline IMemAllocator& getAllocator()
        {
            return *RuntimeStackGuard::getAllocator();
        }
    };

    inline decltype(auto) getRtStackAllocator()
    {
        return RuntimeStackGuard::getAllocator();
    }

    template <typename T>
    using RuntimeStackStdAllocator = StatelessStdAllocator<T, RuntimeStackAllocatorProvider>;

}  // namespace my

// clang-format off
#define rtstack_init(initialSize)  const ::my::RuntimeStackGuard ANONYMOUS_VAR(rtStack__)(initialSize) 

#define rtstack_scope const ::my::RuntimeStackGuard ANONYMOUS_VAR(rtStack__){}

// clang-format on