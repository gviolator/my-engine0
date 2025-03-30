// #my_engine_source_file
#include "my/memory/host_memory.h"
#include "my/threading/barrier.h"
#include "my/utils/scope_guard.h"

namespace my::test
{
    /**
     */
    TEST(TestHostMemory, Allocate)
    {
        using namespace my_literals;

        auto hostMemory = createHostVirtualMemory(mem::AllocationGranularity * 2, true);
        auto pages = hostMemory->allocPages(1_b);

        ASSERT_TRUE(pages);
        ASSERT_EQ(pages.getSize(), hostMemory->getPageSize());
    }

    /**
     */
    TEST(TestHostMemory, AllocateMultiThread)
    {
        constexpr size_t ThreadCount = 15;
        constexpr size_t IterationCount = 250;
        constexpr auto RequireMinimumMemory = mem::PageSize * ThreadCount * IterationCount;

        using MemRegions = std::vector<IHostMemory::MemRegion>;

        std::vector<std::thread> threads;
        std::vector<MemRegions> threadAllocatedRegion;

        threads.reserve(ThreadCount);
        threadAllocatedRegion.reserve(ThreadCount);


        auto hostMemory = createHostVirtualMemory(RequireMinimumMemory, true);

        threading::Barrier barrier(ThreadCount);

        for (size_t i = 0; i < ThreadCount; ++i)
        {
            threads.emplace_back([](threading::Barrier& barrier, HostMemoryPtr hostMemory, MemRegions& pageCollection)
            {
                pageCollection.reserve(IterationCount);
                barrier.enter();

                for (size_t i = 0; i < IterationCount; ++i)
                {
                    pageCollection.emplace_back(hostMemory->allocPages(mem::PageSize));
                }
            }, std::ref(barrier), hostMemory, std::ref(threadAllocatedRegion.emplace_back()));
        }

        for (auto& t : threads)
        {
            t.join();
        }

        std::set<IHostMemory::MemRegion> allRegions;

        for (auto& regionCollection : threadAllocatedRegion)
        {
            for (IHostMemory::MemRegion& region : regionCollection)
            {
                auto [_, emplaceOk] = allRegions.emplace(std::move(region));
                ASSERT_TRUE(emplaceOk);
            }
        }

        auto iter = allRegions.begin();

        const IHostMemory::MemRegion* prevRegion = &*(iter++);
        for (; iter != allRegions.end(); ++iter)
        {
            ASSERT_TRUE(IHostMemory::MemRegion::isAdjacent(*prevRegion, *iter));
            ASSERT_TRUE(IHostMemory::MemRegion::isAdjacent(*iter, *prevRegion));
            prevRegion = &*(iter);
        }
    }

}  // namespace my::test
