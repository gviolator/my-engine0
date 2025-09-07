// #my_engine_source_file

#include "logging_service.h"

#include "my/async/task.h"
#include "my/diag/logging.h"
#include "my/diag/log_sinks.h"

using namespace my::diag;

namespace my
{
    LoggingService::LoggingService()
    {
        if (!hasDefaultLogger())
        {
            setDefaultLogger(createLogger());
        }
        else
        {
            m_needResetDefaultLogger = false;
            mylog_warn("Logger is already set");
        }

        m_sinks.emplace_back(getDefaultLogger().addSink(createConsoleSink()));
        m_sinks.emplace_back(getDefaultLogger().addSink(createPlainTextSink(createDebugOutputStream())));

        // m_logSubscriptions.reserve(2);

        // m_logSubscriptions.push_back(getLogger().subscribe(createDebugOutputLogSubscriber()));
        // m_logSubscriptions.push_back(getLogger().subscribe(createConioOutputLogSubscriber()));
    }

    LoggingService::~LoggingService()
    {
        m_sinks.clear();
        if (m_needResetDefaultLogger)
        {
            diag::setDefaultLogger(nullptr);
        }
    }

    // void LoggingService::addFileOutput(eastl::string_view filename)
    // {
    //     using namespace nau::diag;
    //     m_logSubscriptions.push_back(getLogger().subscribe(createFileOutputLogSubscriber(filename)));
    // }

    async::Task<> LoggingService::shutdownService()
    {
        // if (!diag::hasLogger())
        // {
        //     co_return;
        // }
        co_return;
    }

}  // namespace my