// #my_engine_source_header
#include "my/threading/event.h"

#include "my/diag/check.h"

namespace my::threading_detail
{
    EventBase::EventBase(ResetMode mode) :
        m_mode(mode)
    {
    }

    EventBase::ResetMode EventBase::getMode() const
    {
        return m_mode;
    }
}  // namespace my::threading_detail

#ifdef MY_THREADING_EVENT_NON_OS_IMPL

// clang-format off
#include "my/threading/lock_guard.h"
// clang-format on

namespace my::threading
{
    Event::Event(ResetMode mode, bool signaled) :
        threading_detail::EventBase(mode),
        m_state(signaled)
    {
    }

    Event::~Event() = default;

    void Event::set()
    {
        lock_(m_mutex);

        m_state = true;

        if (m_mode == ResetMode::Auto)
        {
            m_signal.notify_one();
        }
        else
        {
            m_signal.notify_all();
        }
    }

    bool Event::wait(std::optional<std::chrono::milliseconds> timeout)
    {
        std::unique_lock lock{m_mutex};

        bool ready = false;
        if (timeout)
        {
            ready = m_signal.wait_for(lock, *timeout, [this]
            {
                return m_state;
            });
        }
        else
        {
            m_signal.wait(lock, [this]
            {
                return m_state;
            });
            MY_DEBUG_CHECK(m_state);
            ready = true;
        }

        if (ready && m_mode == ResetMode::Auto)
        {
            m_state = false;
        }

        return ready;
    }
}  // namespace my::threading
#endif  // MY_THREADING_EVENT_NON_OS_IMPL
