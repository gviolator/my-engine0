// #my_engine_source_file
#include "my/threading/barrier.h"

#include "my/diag/assert.h"

namespace my::threading
{
    Barrier::Barrier(size_t total) :
        m_total(total)
    {
        MY_DEBUG_ASSERT(m_total > 0);
    }

    bool Barrier::enter(std::optional<std::chrono::milliseconds> timeout)
    {
        const size_t maxExpectedCounter = m_total - 1;
        const size_t counter = m_counter.fetch_add(1);

        MY_DEBUG_ASSERT(counter < m_total);

        if (counter == maxExpectedCounter)
        {
            m_signal.notify_all();
            return true;
        }

        std::unique_lock lock(m_mutex);
        if (!timeout)
        {
            m_signal.wait(lock, [this]
            {
                return m_counter.load() == m_total;
            });
            return true;
        }

        return m_signal.wait_for(lock, std::chrono::milliseconds(timeout->count()), [this]
        {
            return m_counter.load() == m_total;
        });
    }

}  // namespace my::threading
