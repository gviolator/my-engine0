// #my_engine_source_header

#include "my/test/helpers/check_guard.h"
#include "my/threading/barrier.h"
#include "my/threading/lock_guard.h"
#include "my/threading/spin_lock.h"

namespace my::test
{
    /**
        Test: spin lock multithread access.
     */
    TEST(TestSpinLock, MultithreadAccess)
    {
        constexpr size_t ThreadCount = 10;
        constexpr size_t IterationPerThread = 2000;

        std::vector<std::thread> threads;
        threads.reserve(ThreadCount);

        threading::Barrier barrier(ThreadCount);
        threading::SpinLock mutex;

        unsigned counter = 0;

        for (size_t i = 0; i < ThreadCount; ++i)
        {
            threads.emplace_back([&]
            {
                barrier.enter();

                for (unsigned i = 0; i < IterationPerThread; ++i)
                {
                    lock_(mutex);
                    ++counter;
                }
            });
        }

        for (auto& t : threads)
        {
            t.join();
        }

        ASSERT_EQ(counter, ThreadCount * IterationPerThread);
    }

    /**
        Test: SpinLock::lock() cannot be called consecutively multiple times and must cause an assert violation.
     */
    TEST(TestSpinLock, MultipleSubsequentLockMustFail)
    {
        const CheckGuard checkGuard;

        threading::SpinLock mutex;
        mutex.lock();
        ASSERT_EQ(checkGuard.fatalFailureCounter, 0);

        mutex.lock();
        ASSERT_GT(checkGuard.fatalFailureCounter, 0);
    }

    /**
     */
    TEST(TestSpinLock, DestructWhileLockedMustFail)
    {
        const CheckGuard checkGuard;
        {
            threading::SpinLock mutex;
            mutex.lock();
        }
        ASSERT_GT(checkGuard.fatalFailureCounter, 0);
    }

    /**
          Test: recursive spin lock multithread access.
    */
    TEST(TestRecursiveSpinLock, MultithreadAccess)
    {
        constexpr size_t ThreadCount = 10;
        constexpr size_t IterationPerThread = 200;
        constexpr size_t LocksPerIteration = 3;

        std::vector<std::thread> threads;
        threads.reserve(ThreadCount);

        threading::Barrier barrier(ThreadCount);
        threading::RecursiveSpinLock mutex;

        unsigned counter = 0;

        for (size_t i = 0; i < ThreadCount; ++i)
        {
            threads.emplace_back([&]
            {
                barrier.enter();

                for (unsigned i = 0; i < IterationPerThread; ++i)
                {
                    for (size_t j = 0; j < LocksPerIteration; ++j)
                    {
                        mutex.lock();
                    }

                    ++counter;

                    for (size_t j = 0; j < LocksPerIteration; ++j)
                    {
                        mutex.unlock();
                    }
                }
            });
        }

        for (auto& t : threads)
        {
            t.join();
        }

        ASSERT_EQ(counter, ThreadCount * IterationPerThread);
    }

    /**
        Test: RecursiveSpinLock::lock() can be called consecutively multiple times and must NOT cause an assert violation.
     */
    TEST(TestRecursiveSpinLock, MultipleLocks)
    {
        const CheckGuard checkGuard;

        threading::RecursiveSpinLock mutex;
        lock_(mutex);
        lock_(mutex);
        lock_(mutex);

        ASSERT_EQ(checkGuard.fatalFailureCounter, 0);
    }

    /**
     */
    TEST(TestRecursiveSpinLock, DestructWhileLockedMustFail)
    {
        const CheckGuard checkGuard;
        {
            threading::RecursiveSpinLock mutex;
            mutex.lock();
        }
        ASSERT_GT(checkGuard.fatalFailureCounter, 0);
    }
}  // namespace my::test
