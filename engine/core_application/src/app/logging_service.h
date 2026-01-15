// #my_engine_source_file
#pragma once

#include "my/diag/logging.h"
#include "my/rtti/rtti_impl.h"
#include "my/rtti/type_info.h"
#include "my/service/service.h"

namespace my {

class LoggingService : public IServiceShutdown
{
public:
    MY_RTTI_CLASS(LoggingService, IServiceShutdown)

    LoggingService();

    virtual ~LoggingService();

    // virtual void addFileOutput(std::string_view filename);

private:
    async::Task<> shutdownService() override;

    std::vector<diag::LogSinkEntry> m_sinks;
    bool m_needResetDefaultLogger = false;
};

}  // namespace my
