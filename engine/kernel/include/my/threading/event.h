// #my_engine_source_file
#pragma once
#include "my/threading/internal/event_base.h"
#include "my/utils/preprocessor.h"

//#define MY_THREADING_EVENT_NON_OS

#if defined(MY_PLATFORM_WINDOWS) && !defined(MY_THREADING_EVENT_NON_OS)
    #include MY_PLATFORM_HEADER(threading/event.h)
#else

// clang-format off

#pragma once
#include <chrono>
#include <optional>
#include <mutex>

#include "my/kernel/kernel_config.h"

// clang-format on

#define MY_THREADING_EVENT_NON_OS_IMPL

namespace my::threading
{
    /**
     */
    class MY_KERNEL_EXPORT Event final : public threading_detail::EventBase
    {
    public:
        Event(const Event&) = delete;
        Event(Event&&) = delete;

        Event(ResetMode mode = ResetMode::Auto, bool signaled = false);

        Event& operator=(const Event&) = delete;
        Event& operator=(Event&&) = delete;

        ~Event();

        /**
            @brief
                Sets the state of the event to signaled, allowing one or more waiting threads to proceed.
                For an event with reset mode = Auto , the set method releases a single thread. If there are no waiting threads, the wait handle remains signaled until a thread attempts to wait on it, or until its Reset method is called.
        */
        void set();

        void reset();

        /**
            @brief Blocks the current thread until the current WaitHandle receives a signal, using the timeout interval in milliseconds.
            @param[in] timeout timeout
            @returns true if the current event receives a signal; otherwise, false.
        */
        bool wait(std::optional<std::chrono::milliseconds> timeout = std::nullopt);

    protected:
        std::mutex m_mutex;
        std::condition_variable m_signal;
        bool m_state;
    };

}  // namespace my::threading

#endif // 