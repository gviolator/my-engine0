// #my_engine_source_file
#pragma once
#include "my/diag/logging_base.h"
#include "my/containers/intrusive_list.h"

namespace my::diag
{
    class LoggerImpl;

    class LogSinkEntryImpl : public ILogSinkEntry, public IntrusiveListNode<LogSinkEntryImpl>
    {
    public:
        LogSinkEntryImpl(LoggerImpl&, LogSinkPtr&& sink, LogMessageFormatterPtr formatter);

        ~LogSinkEntryImpl();

        void log(const LogMessage& message, std::string_view formattedMessage);

        void resetLogger();

        const LogMessageFormatter* getFormatter() const;

    private:

        void addFilter(LogFilterPtr) override;
        void removeFilter(const LogFilter&) override;

        LoggerImpl* m_logger;
        const LogSinkPtr m_sink;
        const LogMessageFormatterPtr m_formatter;
        std::list<LogFilterPtr> m_filters;
    };
}
