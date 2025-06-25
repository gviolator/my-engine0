// #my_engine_source_file
#if !defined(__linux__)
#include <intrin.h>

#include "my/memory/fixed_size_block_allocator.h"

namespace my::test
{
    namespace
    {
        struct CustomAlignedObject
        {
            __m128 value[4];

            CustomAlignedObject()
            {
                value[0] = _mm_setzero_ps();
            }
        };
    }  // namespace

  

    TEST(FixedSizeAllocator, AllocAligned)
    {
        using namespace my_literals;
        
        //BlockAllocatorAdataper adapter {createHostVirtualMemory(1_Mb, true), true};


        auto allocator = createFixedSizeBlockAllocator(createHostVirtualMemory(1_Mb, true), sizeof(CustomAlignedObject), true);

        auto ptr = allocator->alloc(sizeof(CustomAlignedObject));
        ASSERT_TRUE(reinterpret_cast<uintptr_t>(ptr) % alignof(CustomAlignedObject) == 0);

        [[maybe_unused]] auto obj = new(ptr) CustomAlignedObject;
    }
}  // namespace my::test
#endif