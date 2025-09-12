#include "my/memory/allocator.h"

namespace my::test {

class TestGenericAllocator : public testing::TestWithParam<bool>
{
};

TEST_P(TestGenericAllocator, SimpleAllocation)
{
    constexpr std::array BlockSize = {8, 16, 256, 1024, 2048, 4096};
    constexpr size_t IterCount = 1'000;
    constexpr size_t RequiredAlignment = 16;

    const bool threadSafe = this->GetParam();

    const AllocatorPtr allocator = createDefaultGenericAllocator(threadSafe);
    ASSERT_NE(allocator, nullptr);
    EXPECT_GE(allocator->getMaxAlignment(), RequiredAlignment);

    for (size_t i = 0; i < IterCount; ++i)
    {
        std::array<void*, BlockSize.size()> ptrs;
        for (size_t s = 0; s < BlockSize.size(); ++s)
        {
            void* const ptr = allocator->alloc(BlockSize[s]);
            ASSERT_NE(ptr, nullptr);
            EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % RequiredAlignment, 0);
            ptrs[s] = ptr;
        }

        for (void* const ptr : ptrs)
        {
            allocator->free(ptr);
        }
    }
}

TEST_P(TestGenericAllocator, StdContainer)
{
    using Container = std::pmr::vector<std::pmr::string>;
    constexpr size_t ContainerSize = 2'000;

    const bool threadSafe = this->GetParam();
    const AllocatorPtr allocator = createDefaultGenericAllocator(threadSafe);
    std::pmr::memory_resource* const memResource = allocator->getMemoryResource();
    ASSERT_NE(memResource, nullptr);

    Container container{memResource};
    size_t counter = 0;
    for (; container.size() < ContainerSize;)
    {
        container.emplace_back(std::format("Test Text [{}]", ++counter));
    }
}

TEST_P(TestGenericAllocator, MultiThread)
{
    const bool threadSafe = this->GetParam();
    if (!threadSafe)
    {
        return;
    }
}

INSTANTIATE_TEST_SUITE_P(Default, TestGenericAllocator, testing::Values(true, false));

}  // namespace my::test
