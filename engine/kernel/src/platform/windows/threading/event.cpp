// #my_engine_source_file
#include "my/platform/windows/threading/event.h"

#include "my/diag/assert.h"

namespace my::threading
{
    static_assert(sizeof(void*) == sizeof(HANDLE) && std::is_same_v<void*, HANDLE>);

    Event::Event(ResetMode mode, bool signaled) :
        threading_detail::EventBase(mode),
        m_hEvent(::CreateEventW(nullptr, static_cast<BOOL>(mode == ResetMode::Manual), static_cast<BOOL>(signaled), nullptr))
    {
        MY_DEBUG_ASSERT(m_hEvent);
    }

    Event::~Event()
    {
        if (m_hEvent)
        {
            ::CloseHandle(m_hEvent);
        }
    }

    void Event::set()
    {
        ::SetEvent(m_hEvent);
    }

    void Event::reset()
    {
        ::ResetEvent(m_hEvent);
    }

    bool Event::wait(std::optional<std::chrono::milliseconds> timeout)
    {
        const DWORD ms = timeout ? static_cast<DWORD>(timeout->count()) : INFINITE;
        const DWORD waitResult = ::WaitForSingleObject(m_hEvent, ms);

        if (waitResult == WAIT_OBJECT_0)
        {
            return true;
        }

        MY_DEBUG_ASSERT(waitResult == WAIT_TIMEOUT);

        return false;
    }
}  // namespace my::threading
