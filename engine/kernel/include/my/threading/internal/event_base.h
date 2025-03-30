// #my_engine_source_file
#pragma once
#include "my/kernel/kernel_config.h"

namespace my::threading_detail
{
    class MY_KERNEL_EXPORT EventBase
    {
    public:
        enum class ResetMode
        {
            Auto,
            Manual
        };

        /**
            @brief Get the reset mode of this event. Can be 'Auto' or 'Manual'.
            @returns Reset mode for current event instance.
        */
        ResetMode getMode() const;


    protected:
        EventBase(const EventBase&) = delete;
        EventBase(EventBase&&) = delete;

        EventBase(ResetMode mode);

        EventBase& operator=(const EventBase&) = delete;
        EventBase& operator=(EventBase&&) = delete;


        const ResetMode m_mode;
    };
}  // namespace my::threadin_detail
