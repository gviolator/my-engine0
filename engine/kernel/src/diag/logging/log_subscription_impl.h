// #my_engine_source_header
#pragma once
#include "my/diag/logging_base.h"
#include "my/containers/intrusive_list.h"

namespace my::diag
{
    class LoggerImpl;

    class LogSubscriptionImpl : public LogSubscriptionApi, public IntrusiveListNode<LogSubscriptionImpl>
    {
    public:
        LogSubscriptionImpl(LoggerImpl&, LogSubscriberPtr&& subscriber);

        ~LogSubscriptionImpl();

        void log(const LogMessage& message);

        void resetLogger();

    private:

        void addFilter(LogFilterPtr) override;
        void removeFilter(const LogFilter&) override;

        LoggerImpl* m_logger;
        const LogSubscriberPtr m_subscriber;
        std::list<LogFilterPtr> m_filters;
    };
}