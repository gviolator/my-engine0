// #my_engine_source_file
#pragma once

#include <synchapi.h>
#if MY_DEBUG
    #include <thread>
#endif

#include "my/diag/check.h"
#include "my/kernel/kernel_config.h"
#include "my/threading/thread_safe_annotations.h"

namespace my::threading
{
    /**
     */
    class THREAD_CAPABILITY("mutex") CriticalSection
    {
    public:
        CriticalSection()
        {
            InitializeCriticalSection(&m_cs);
        }

        ~CriticalSection()
        {
            MY_DEBUG_FATAL(m_threadOwner == std::thread::id{}, "CriticalSection still locked");
            DeleteCriticalSection(&m_cs);
        }

        CriticalSection(const CriticalSection&) = delete;
        CriticalSection(CriticalSection&&) = delete;
        CriticalSection& operator=(const CriticalSection&) = delete;
        CriticalSection& operator=(CriticalSection&&) = delete;

#if MY_DEBUG_CHECK_ENABLED
        void lock() THREAD_ACQUIRE()
        {
            EnterCriticalSection(&m_cs);

            MY_DEBUG_FATAL(m_threadOwner == std::thread::id{}, "Recursive lock critical section");
            m_threadOwner = std::this_thread::get_id();
        }

        void unlock() THREAD_RELEASE()
        {
            MY_DEBUG_FATAL(m_threadOwner == std::this_thread::get_id(), "Invalid thread");
            m_threadOwner = {};

            LeaveCriticalSection(&m_cs);
        }
#else
        void lock() THREAD_ACQUIRE()
        {
            EnterCriticalSection(&m_cs);
        }

        void unlock() THREAD_RELEASE()
        {
            LeaveCriticalSection(&m_cs);
        }
#endif

    private:
        ::CRITICAL_SECTION m_cs;

#if MY_DEBUG_CHECK_ENABLED
        std::thread::id m_threadOwner = {};
#endif
    };

    /**
     */
    class THREAD_CAPABILITY("mutex") RecursiveCriticalSection
    {
    public:
        RecursiveCriticalSection()
        {
            InitializeCriticalSection(&m_cs);
        }

        ~RecursiveCriticalSection()
        {
            MY_DEBUG_FATAL(m_threadOwner == std::thread::id{} && m_lockCounter == 0, "RecursiveCriticalSection still locked");
            DeleteCriticalSection(&m_cs);
        }

        RecursiveCriticalSection(const RecursiveCriticalSection&) = delete;
        RecursiveCriticalSection(RecursiveCriticalSection&&) = delete;
        RecursiveCriticalSection& operator=(const RecursiveCriticalSection&) = delete;
        RecursiveCriticalSection& operator=(RecursiveCriticalSection&&) = delete;

#if MY_DEBUG_CHECK_ENABLED
        void lock() THREAD_ACQUIRE()
        {
            EnterCriticalSection(&m_cs);

            const auto ThisThreadId = std::this_thread::get_id();

            MY_DEBUG_FATAL(++m_lockCounter == 1 || m_threadOwner == ThisThreadId);
            m_threadOwner = std::this_thread::get_id();
        }

        void unlock() THREAD_RELEASE()
        {
            MY_DEBUG_FATAL(m_threadOwner == std::this_thread::get_id(), "Invalid thread");
            MY_DEBUG_FATAL(m_lockCounter > 0);
            if (--m_lockCounter == 0)
            {
                m_threadOwner = {};
            }

            LeaveCriticalSection(&m_cs);
        }
#else
        void lock() THREAD_ACQUIRE()
        {
            EnterCriticalSection(&m_cs);
        }

        void unlock() THREAD_RELEASE()
        {
            LeaveCriticalSection(&m_cs);
        }
#endif

    private:
        ::CRITICAL_SECTION m_cs;

#if MY_DEBUG_CHECK_ENABLED
        std::thread::id m_threadOwner = {};
        uint32_t m_lockCounter = 0;
#endif
    };

}  // namespace my::threading
