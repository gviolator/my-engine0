// #my_engine_source_file
#include "log_sink_entry.h"

#include "logger_impl.h"

namespace my::diag
{
    LogSinkEntryImpl::LogSinkEntryImpl(LoggerImpl& logger, LogSinkPtr&& sink, LogMessageFormatterPtr formatter) :
        m_logger{&logger},
        m_sink{std::move(sink)},
        m_formatter{std::move(formatter)}
    {
        MY_DEBUG_FATAL(m_sink);
    }

    LogSinkEntryImpl::~LogSinkEntryImpl()
    {
        if (m_logger)
        {
            m_logger->releaseLogSink(*this);
        }
    }

    void LogSinkEntryImpl::resetLogger()
    {
        m_logger = nullptr;
    }

    const LogMessageFormatter* LogSinkEntryImpl::getFormatter() const
    {
        return m_formatter.get();
    }

    void LogSinkEntryImpl::log(const LogMessage& logMessage, std::string_view formattedMessage)
    {
        const bool shouldLog = std::all_of(m_filters.begin(), m_filters.end(), [&logMessage](const LogFilterPtr& filter)
        {
            return filter->shouldLog(logMessage);
        });

        if (shouldLog)
        {
            m_sink->log(logMessage, formattedMessage);
        }
    }

    void LogSinkEntryImpl::addFilter(LogFilterPtr)
    {
    }

    void LogSinkEntryImpl::removeFilter(const LogFilter&)
    {
    }

}  // namespace my::diag
