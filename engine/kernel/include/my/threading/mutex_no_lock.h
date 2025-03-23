// #my_engine_source_header
#pragma once
#include "my/threading/thread_safe_annotations.h"

namespace my::threading
{
    /**
     */
    class THREAD_CAPABILITY("mutex") NoLockMutex
    {
    public:
        NoLockMutex() = default;
        NoLockMutex(const NoLockMutex&) = delete;
        NoLockMutex& operator=(const NoLockMutex&) = delete;

        void lock() THREAD_ACQUIRE()
        {
        }

        void unlock() THREAD_RELEASE()
        {
        }
    };

}  // namespace my::threading
