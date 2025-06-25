// #my_engine_source_file
#pragma once
#include <shared_mutex>

#include "log_sink_entry.h"
#include "my/diag/logging.h"
#include "my/memory/singleton_memop.h"
#include "my/rtti/rtti_impl.h"

namespace my::diag
{

    class LoggerImpl final : public Logger
    {
    public:
        MY_REFCOUNTED_CLASS(LoggerImpl, Logger)

        LoggerImpl();
        ~LoggerImpl();

        void releaseLogSink(LogSinkEntryImpl&);

    private:
        void setName(std::string name) override;

        const std::string& getName() const override;

        void log(LogLevel level, SourceInfo sourceInfo, LogContextPtr context, std::string message) override;

        LogSinkEntry addSink(LogSinkPtr sink, LogMessageFormatterPtr customFormatter) override;

        void addFilter(LogFilterPtr) override;

        void removeFilter(const LogFilter&) override;

        void setDefaultFormatter(LogMessageFormatterPtr formatter) override;

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
        IntrusiveList<LogSinkEntryImpl> m_sinks;
        std::shared_mutex m_mutex;
        LogMessageFormatterPtr m_defaultFormatter;
    };

}  // namespace my::diag