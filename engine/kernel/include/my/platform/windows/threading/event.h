// #my_engine_source_header
#pragma once
#include <chrono>
#include <optional>

#include "my/kernel/kernel_config.h"
#include "my/threading/internal/event_base.h"

namespace my::threading
{

    /**
     */
    class MY_KERNEL_EXPORT Event : public threading_detail::EventBase
    {
    public:
        Event(ResetMode mode = ResetMode::Auto, bool signaled = false);
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

    private:
        void* const m_hEvent;
    };

}  // namespace my::threading
