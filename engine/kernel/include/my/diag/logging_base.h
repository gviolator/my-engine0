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

    /**
     */
    struct MY_ABSTRACT_TYPE LogSubscriber : IRefCounted
    {
        MY_INTERFACE(my::diag::LogSubscriber, IRefCounted)

        virtual void log(const LogMessage& message) = 0;
    };

    using LogSubscriberPtr = my::Ptr<LogSubscriber>;

    /**
     */
    struct MY_ABSTRACT_TYPE LogFilter : IRefCounted
    {
        MY_INTERFACE(my::diag::LogFilter, IRefCounted)

        virtual bool shouldLog(const LogMessage& message) = 0;
    };

    struct MY_ABSTRACT_TYPE LogLevelFilter : LogFilter
    {
        MY_INTERFACE(LogLevelFilter, LogFilter)

        virtual void setLevel(LogLevel level) = 0;
    };

    using LogFilterPtr = my::Ptr<LogFilter>;

    /**
     */
    struct ILogSubscription
    {
        virtual ~ILogSubscription() = default;

        virtual void addFilter(LogFilterPtr) = 0;

        virtual void removeFilter(const LogFilter&) = 0;
    };

    using LogSubscription = std::unique_ptr<ILogSubscription>;

    // class MY_KERNEL_EXPORT [[nodiscard]] LogSubscription
    // {
    // public:
    //     LogSubscription();
    //     LogSubscription(LogSubscription&&);
    //     LogSubscription(const LogSubscription&) = delete;
    //     ~LogSubscription();

    //     LogSubscription& operator=(LogSubscription&&);
    //     LogSubscription& operator=(const LogSubscription&) = delete;
    //     inline LogSubscription& operator=(std::nullptr_t)
    //     {
    //         release();
    //         return *this;
    //     }

    //     explicit operator bool() const;

    //     void release();
    //     void addFilter(LogFilterPtr);
    //     void removeFilter(const LogFilter&);

    // private:
    //     // LogSubscription(Logger::Ptr&&, uint32_t);

    //     // std::weak_ptr<Logger> m_logger;
    //     uint32_t m_id = 0;

    //     friend struct Logger;
    // };

    /**
     */
    struct MY_ABSTRACT_TYPE Logger : IRefCounted
    {
        MY_INTERFACE(my::diag::Logger, IRefCounted)

        virtual void setName(std::string name) = 0;

        virtual const std::string& getName() const = 0;

        virtual void log(LogLevel level, SourceInfo sourceInfo, LogContextPtr context, std::string message) = 0;

        virtual LogSubscription subscribe(LogSubscriberPtr) = 0;

        virtual void addFilter(LogFilterPtr) = 0;

        virtual void removeFilter(const LogFilter&) = 0;
    };

    using LoggerPtr = my::Ptr<Logger>;
}  // namespace my::diag
