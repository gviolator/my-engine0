// #my_engine_source_file
#include "my/memory/runtime_stack.h"
#include "my/test/helpers/check_guard.h"

using namespace my::my_literals;

namespace my::test
{
    TEST(TestRuntimeStack, InitAllocator)
    {
        rtstack_init(2_Mb);
        EXPECT_TRUE(GetRtStackAllocator().is<IStackAllocatorInfo>());
    }

    TEST(TestRuntimeStack, GetAllocatorInScope)
    {
        rtstack_init(2_Mb);
        {
            rtstack_scope;

            EXPECT_TRUE(GetRtStackAllocator().is<IStackAllocatorInfo>());
        }
    }

    TEST(TestRuntimeStack, GetAllocatorNoInit)
    {
        EXPECT_FALSE(GetRtStackAllocator().is<IStackAllocatorInfo>());
    }

    TEST(TestRuntimeStack, Allocate)
    {
        const CheckGuard noAssert{};
        rtstack_init(2_Mb);

        
        auto ptr0 = GetRtStackAllocator().alloc(128);
        EXPECT_NE(ptr0, nullptr);
        EXPECT_TRUE(reinterpret_cast<uintptr_t>(ptr0) % alignof(std::max_align_t) == 0);

        auto ptr1 = GetRtStackAllocator().alloc(128);
        EXPECT_NE(ptr1, nullptr);
        EXPECT_TRUE(reinterpret_cast<uintptr_t>(ptr1) % alignof(std::max_align_t) == 0);

        EXPECT_NE(ptr0, ptr1);
    }

    TEST(TestRuntimeStack, AllocateAligned)
    {
        const CheckGuard noAssert{};
        rtstack_init(2_Mb);

        constexpr size_t DefaultAlignment = alignof(std::max_align_t);
        constexpr size_t Alignment0 = 4;
        constexpr size_t Alignment1 = 8;
        constexpr size_t Alignment2 = 16;
        constexpr size_t AllocCount = 100;

        auto& allocator = GetRtStackAllocator();

        for (size_t i = 0; i < AllocCount; ++i)
        {
            const auto ptr0 = reinterpret_cast<uintptr_t>(allocator.alloc(Alignment0 * 5, Alignment0));
            ASSERT_NE(ptr0, 0);
            ASSERT_TRUE(ptr0 % Alignment0 == 0);

            const auto ptr1 = reinterpret_cast<uintptr_t>(allocator.alloc(Alignment1 * 5, Alignment1));
            ASSERT_NE(ptr1, 0);
            ASSERT_TRUE(ptr1 % Alignment1 == 0);

            const auto ptrX = reinterpret_cast<uintptr_t>(allocator.alloc(DefaultAlignment));
            ASSERT_NE(ptrX, 0);
            ASSERT_TRUE(ptrX % DefaultAlignment == 0);

            const auto ptr2 = reinterpret_cast<uintptr_t>(allocator.alloc(Alignment2 * 5, Alignment2));
            ASSERT_NE(ptr2, 0);
            ASSERT_TRUE(ptr2 % Alignment2 == 0);
        }
    }

    TEST(TestRuntimeStack, AllocationScope)
    {
        const CheckGuard noAssert{};

        const auto getStackOffset = []
        {
            return GetRtStackAllocator().as<const IStackAllocatorInfo&>().getAllocationOffset();
        };

        rtstack_init(2_Mb);

        [[maybe_unused]] void* const ptr0 = GetRtStackAllocator().alloc(128);

        const uintptr_t top0 = getStackOffset();

        {
            rtstack_scope;
            [[maybe_unused]] void* const ptr1 = GetRtStackAllocator().alloc(128);
            ASSERT_NE(top0, getStackOffset());
        }

        ASSERT_EQ(top0, getStackOffset());
    }

    TEST(TestRuntimeStack, StdWrapper)
    {
        const CheckGuard noAssert{};
        using String = std::basic_string<char, std::char_traits<char>, RtStackStdAllocator<char>>;
        using List = std::list<String, RtStackStdAllocator<String>>;

        rtstack_init(2_Mb);

        List list0;
        list0.emplace_back("text1");
        list0.emplace_back("text2");
        list0.emplace_back("text3");
        {
            auto el = list0.begin();
            ASSERT_EQ(*(el++), "text1");
            ASSERT_EQ(*(el++), "text2");
            ASSERT_EQ(*el, "text3");
        }

        List list1 = std::move(list0);
        {
            auto el = list1.begin();
            ASSERT_EQ(*(el++), "text1");
            ASSERT_EQ(*(el++), "text2");
            ASSERT_EQ(*el, "text3");
        }
    }

    TEST(TestRuntimeStack, AllocateMultiplePages)
    {
        const CheckGuard noAssert{};
        rtstack_init(2_Mb);

        constexpr size_t AllocationSize = mem::PageSize * 4;
        constexpr size_t AllocationCount = (mem::AllocationGranularity / AllocationSize) * 10;

        for (size_t i = 0; i < AllocationCount; ++i)
        {
            void* const ptr = GetRtStackAllocator().alloc(AllocationSize);
            ASSERT_NE(ptr, nullptr);
            memset(ptr, 0, AllocationSize);
        }
    }

}  // namespace my::test
