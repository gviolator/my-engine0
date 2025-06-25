// #my_engine_source_file
#pragma once
#include <ctime>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "my/diag/source_info.h"
#include "my/kernel/kernel_config.h"
#include "my/rtti/ptr.h"
#include "my/rtti/type_info.h"
#include "my/utils/runtime_enum.h"
#include "my/utils/typed_flag.h"

namespace my::diag
{
    /**
     */
    MY_DEFINE_ENUM_(LogLevel,
                    Verbose = FlagValue(1),
                    Debug = FlagValue(2),
                    Info = FlagValue(3),
                    Warning = FlagValue(4),
                    Error = FlagValue(5),
                    Critical = FlagValue(6))

    MY_DEFINE_TYPED_FLAG(LogLevel)

    /**
     */
    struct LogContext
    {
        std::vector<std::string> tags;
    };

    using LogContextPtr = std::shared_ptr<LogContext>;

    /**
     */
    struct LogMessage
    {
        // uint32_t index;
        std::thread::id threadId;
        std::time_t timeStamp;
        LogLevel level;
        SourceInfo source;
        std::string text;
        LogContextPtr context;
    };



    struct MY_ABSTRACT_TYPE LogMessageFormatter : IRefCounted
    {
        MY_INTERFACE(my::diag::LogMessageFormatter, IRefCounted);

        virtual std::string formatMessage(const LogMessage&) const = 0;
    };

    using LogMessageFormatterPtr = Ptr<LogMessageFormatter>;

    /**
     */
    struct MY_ABSTRACT_TYPE LogSink : IRefCounted
    {
        MY_INTERFACE(my::diag::LogSink, IRefCounted)

        virtual void log(const LogMessage& sourceMessage, std::string_view formattedMessage) = 0;
    };

    using LogSinkPtr = my::Ptr<LogSink>;

    /**
     */
    struct MY_ABSTRACT_TYPE LogFilter : IRefCounted
    {
        MY_INTERFACE(my::diag::LogFilter, IRefCounted)

        virtual bool shouldLog(const LogMessage& message) = 0;
    };

    using LogFilterPtr = my::Ptr<LogFilter>;



    /**
     */
    struct ILogSinkEntry
    {
        virtual ~ILogSinkEntry() = default;

        virtual void addFilter(LogFilterPtr) = 0;

        virtual void removeFilter(const LogFilter&) = 0;
    };

    using LogSinkEntry = std::unique_ptr<ILogSinkEntry>;

    /**
     */
    struct MY_ABSTRACT_TYPE Logger : IRefCounted
    {
        MY_INTERFACE(my::diag::Logger, IRefCounted)

        virtual void setName(std::string name) = 0;

        virtual const std::string& getName() const = 0;

        virtual void log(LogLevel level, SourceInfo sourceInfo, LogContextPtr context, std::string message) = 0;

        virtual LogSinkEntry addSink(LogSinkPtr, LogMessageFormatterPtr = nullptr) = 0;

        virtual void addFilter(LogFilterPtr) = 0;

        virtual void removeFilter(const LogFilter&) = 0;

        virtual void setDefaultFormatter(LogMessageFormatterPtr formatter) = 0;
    };

    using LoggerPtr = my::Ptr<Logger>;
}  // namespace my::diag
