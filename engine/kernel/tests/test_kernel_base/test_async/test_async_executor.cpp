// #my_engine_source_file
#include "my/async/thread_pool_executor.h"


namespace my::test
{
    using namespace testing;
    using namespace std::chrono_literals;

    using ExecutorFactory = async::ExecutorPtr (*)();

    /**
     */
    class TestAsyncExecutor : public testing::Test,
                              public testing::WithParamInterface<ExecutorFactory>
    {
    protected:
        static constexpr size_t ThreadsCount = 8;

        auto createExecutor() const
        {
            auto factory = GetParam();
            return factory();
        }

        static void waitWorks(async::ExecutorPtr executor)
        {
            executor->waitAnyActivity();
        }
    };

    /**
     */
    TEST_P(TestAsyncExecutor, Execute)
    {
        constexpr size_t JobsCount = 200'000;

        std::atomic_size_t counter = 0;

        auto executor = createExecutor();

        for(size_t i = 0; i < JobsCount; ++i)
        {
            executor->execute([](void* counterPtr, void*) noexcept
                            {
                                reinterpret_cast<std::atomic_size_t*>(counterPtr)->fetch_add(1);
                            },
                            &counter);
        }

        waitWorks(executor);

        ASSERT_THAT(counter, Eq(JobsCount));
    }

    const ExecutorFactory createDefaultPoolExecutor = []
    {
        return async::createThreadPoolExecutor();
    };

    const ExecutorFactory createDagPoolExecutor = []
    {
        //return async::createDagThreadPoolExecutor(true);
        return async::createThreadPoolExecutor();
    };

    INSTANTIATE_TEST_SUITE_P(Default,
                             TestAsyncExecutor,
                             testing::Values(createDefaultPoolExecutor, createDagPoolExecutor));

}  // namespace my::test
