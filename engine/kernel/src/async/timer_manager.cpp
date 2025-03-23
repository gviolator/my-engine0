// #my_engine_source_header
#include "my/async/async_timer.h"

namespace my::async
{
    using TimerManagerPtr = std::unique_ptr<ITimerManager>;

    namespace
    {
        TimerManagerPtr& getTimerManagerInstanceRef()
        {
            static TimerManagerPtr s_timerManagerInstance;
            return (s_timerManagerInstance);
        }
    }  // namespace

    void ITimerManager::setInstance(TimerManagerPtr instance)
    {
        MY_DEBUG_CHECK(!instance || !getTimerManagerInstanceRef(), "Timer manager instance already set");

        getTimerManagerInstanceRef() = std::move(instance);
    }

    ITimerManager& ITimerManager::getInstance()
    {
        auto& instance = getTimerManagerInstanceRef();
        MY_DEBUG_FATAL(instance);

        return *instance;
    }

    bool ITimerManager::hasInstance()
    {
        return static_cast<bool>(getTimerManagerInstanceRef());
    }

}  // namespace my::async
