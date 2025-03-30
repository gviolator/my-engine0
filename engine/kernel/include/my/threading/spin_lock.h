// #my_engine_source_file


#pragma once
#include "my/diag/check.h"
#include "my/kernel/kernel_config.h"
#include "my/threading/thread_safe_annotations.h"
#include "my/utils/scope_guard.h"

#if defined(MY_THREADS_ENABLED)

// clang-format off
#include <atomic>
#include <thread>
// clang-format on

namespace my::threading
{
    /**
        @brief:
            Before using SpinLock, read this: https://www.realworldtech.com/forum/?threadid=189711&curpostid=189723
    */
    class THREAD_CAPABILITY("mutex") SpinLock
    {
        static_assert(std::atomic<std::thread::id>::is_always_lock_free);

    public:
        SpinLock() = default;
        SpinLock(SpinLock&&) = delete;
        SpinLock(const SpinLock&) = delete;
        SpinLock operator=(const SpinLock&) = delete;

        ~SpinLock()
        {
            MY_DEBUG_FATAL(m_threadOwner.load(std::memory_order_relaxed) == std::thread::id{}, "Mutex is locked while destructed");
        }

        inline void lock() THREAD_ACQUIRE()
        {
            const std::thread::id NoThread{};
            const std::thread::id ThisThread = std::this_thread::get_id();

#if MY_DEBUG_CHECK_ENABLED
            // Check same thread lock twice (which is not supported by this kind of mutex)
            if (m_threadOwner.load(std::memory_order_relaxed) == ThisThread)
            {
                MY_DEBUG_FATAL_FAILURE("Recursive mutex acquisition is not allowed");
                return;
            }
#endif

            while(true)
            {
                auto expectedValue = NoThread;
                if(m_threadOwner.compare_exchange_strong(expectedValue, ThisThread, std::memory_order_seq_cst, std::memory_order_acquire))
                {
                    break;
                }

                // Reducing cache coherency traffic:
                // only a single CPU core may write to a cache line but multiple CPU cores may simultaneously read from it.
                // The atomic exchange operation requires write access that makes a problem
                // when more than one thread is spinning trying to acquire the lock.
                // So spin in read-only relaxed load op.
                while(m_threadOwner.load(std::memory_order_relaxed) != NoThread)
                {
                    std::this_thread::yield();
                }
            }
        }

        inline void unlock() THREAD_RELEASE()
        {
            MY_DEBUG_FATAL(m_threadOwner.load(std::memory_order_relaxed) == std::this_thread::get_id());
            m_threadOwner.store(std::thread::id{}, std::memory_order_release);
        }

    private:
        std::atomic<std::thread::id> m_threadOwner{};
    };

    /**
     */
    class THREAD_CAPABILITY("mutex") RecursiveSpinLock
    {
        static_assert(std::atomic<std::thread::id>::is_always_lock_free);

    public:
        RecursiveSpinLock() = default;
        RecursiveSpinLock(RecursiveSpinLock&&) = delete;
        RecursiveSpinLock(const RecursiveSpinLock&) = delete;
        RecursiveSpinLock operator=(const RecursiveSpinLock&) = delete;

        ~RecursiveSpinLock()
        {
            MY_DEBUG_CHECK(m_threadOwner == std::thread::id{}, "Mutex is locked while destructed");
            MY_DEBUG_CHECK(m_lockCounter == 0);
        }

        inline void lock() THREAD_ACQUIRE()
        {
            const std::thread::id NoThread{};
            const std::thread::id ThisThread = std::this_thread::get_id();

            scope_on_leave
            {
                MY_DEBUG_CHECK(m_threadOwner == ThisThread);
                ++m_lockCounter;
            };

            auto expectedValue = m_threadOwner.load(std::memory_order_acquire);
            if(expectedValue == ThisThread)
            {
                MY_DEBUG_CHECK(m_lockCounter > 0);
                return;
            }

            while(true)
            {
                expectedValue = NoThread;
                if(m_threadOwner.compare_exchange_strong(expectedValue, ThisThread, std::memory_order_seq_cst, std::memory_order_acquire))
                {
                    MY_DEBUG_CHECK(m_lockCounter == 0);
                    break;
                }

                while(m_threadOwner.load(std::memory_order_relaxed) != NoThread)
                {
                    std::this_thread::yield();
                }
            }
        }

        inline void unlock() THREAD_RELEASE()
        {
            MY_DEBUG_CHECK(m_threadOwner == std::this_thread::get_id());
            MY_DEBUG_CHECK(m_lockCounter > 0);

            if(--m_lockCounter == 0)
            {
                m_threadOwner.store(std::thread::id{}, std::memory_order_release);
            }
        }

    private:
        std::atomic<std::thread::id> m_threadOwner{};
        size_t m_lockCounter = 0;
    };

}  // namespace my::threading

#else

namespace my::threading
{

    class THREAD_CAPABILITY("mutex") SpinLock
    {
    public:
        SpinLock() = default;
        SpinLock(const SpinLock&) = delete;
        SpinLock operator=(const SpinLock&) = delete;

        void lock() THREAD_ACQUIRE()
        {
        }

        void unlock() THREAD_RELEASE()
        {
        }
    };

    using RecursiveSpinLock = SpinLock;

}  // namespace Runtime::Threading

#endif  // RUNTIME_THREADS
