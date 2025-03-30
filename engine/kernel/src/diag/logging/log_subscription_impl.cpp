// #my_engine_source_file
#include "log_subscription_impl.h"

#include "logger_impl.h"


namespace my::diag
{
    LogSubscriptionImpl::LogSubscriptionImpl(LoggerImpl& logger, LogSubscriberPtr&& subscriber) :
        m_logger(&logger),
        m_subscriber(std::move(subscriber))
    {
        MY_DEBUG_FATAL(m_subscriber);
    }

    LogSubscriptionImpl::~LogSubscriptionImpl()
    {
        if (m_logger)
        {
            m_logger->releaseSubscription(*this);
        }
    }

    void LogSubscriptionImpl::resetLogger()
    {
        m_logger = nullptr;
    }

    void LogSubscriptionImpl::log(const LogMessage& message)
    {
        const bool shouldLog = std::all_of(m_filters.begin(), m_filters.end(), [&message](const LogFilterPtr& filter)
        {
            return filter->shouldLog(message);
        });

        if (shouldLog)
        {
            m_subscriber->log(message);
        }
    }

    void LogSubscriptionImpl::addFilter(LogFilterPtr)
    {
    }

    void LogSubscriptionImpl::removeFilter(const LogFilter&)
    {
    }


}  // namespace my::diag
