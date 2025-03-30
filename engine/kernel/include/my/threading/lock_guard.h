// #my_engine_source_file

#pragma once

#include <mutex>
#include <shared_mutex>

#include "my/threading/thread_safe_annotations.h"
#include "my/utils/preprocessor.h"

namespace my::threading
{

    template <typename T>
    class THREAD_SCOPED_CAPABILITY LockGuard : protected std::lock_guard<T>
    {
    public:
        explicit LockGuard(T& mutex) THREAD_ACQUIRE(mutex) :
            std::lock_guard<T>(mutex)
        {
        }

        explicit LockGuard(T& mutex, std::adopt_lock_t) THREAD_REQUIRES(mutex) :
            std::lock_guard<T>(mutex, std::adopt_lock)
        {
        }

        ~LockGuard() THREAD_RELEASE() = default;
    };

}  // namespace my::threading

// clang-format off
#define lock_(Mutex) \
    ::my::threading::LockGuard ANONYMOUS_VAR(lock_mutex_) {Mutex}

#define shared_lock_(Mutex) \
    ::std::shared_lock ANONYMOUS_VAR(lock_mutex_) {Mutex}

// clang-format on