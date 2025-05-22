// #my_engine_source_file
#pragma once
#include <shared_mutex>

#include "log_subscription_impl.h"
#include "my/diag/logging.h"
#include "my/memory/singleton_memop.h"
#include "my/rtti/rtti_impl.h"


namespace my::diag
{

    class LoggerImpl final : public Logger
    {
    public:
        MY_REFCOUNTED_CLASS(LoggerImpl, Logger)

        ~LoggerImpl();

        void releaseSubscription(LogSubscriptionImpl&);

    private:
        void setName(std::string name) override;

        const std::string& getName() const override;

        void log(LogLevel level, SourceInfo sourceInfo, LogContextPtr context, std::string message) override;

        LogSubscription subscribe(LogSubscriberPtr) override;

        void addFilter(LogFilterPtr) override;

        void removeFilter(const LogFilter&) override;

#if 0
        struct SubscriberEntry
        {
            ILogSubscriber::Ptr subscriber;
            ILogMessageFilter::Ptr filter;
            uint32_t id;

            SubscriberEntry(ILogSubscriber::Ptr inSubscriber, ILogMessageFilter::Ptr inFilter, uint64_t inIndex) :
                subscriber(std::move(inSubscriber)),
                filter(std::move(inFilter)),
                id(inIndex)
            {
            }

            inline void operator()(const LoggerMessage& message) const
            {
                // assert subscriber
                if (!filter || filter->acceptMessage(message))
                {
                    subscriber->processMessage(message);
                }
            }
        };

        std::shared_mutex m_mutex;
        std::atomic_uint32_t m_messageIndex = 0;
        uint32_t m_subscriberId = 0;
        std::list<SubscriberEntry> m_subscribers;
#endif
        std::string m_name;
        IntrusiveList<LogSubscriptionImpl> m_subscriptions;
        std::shared_mutex m_mutex;
    };

}  // namespace my::diag