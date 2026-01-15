// #my_engine_source_file
#include "logger_impl.h"

#include "my/memory/runtime_stack.h"
#include "my/threading/lock_guard.h"
#include "my/utils/scope_guard.h"


template <>
struct std::formatter<my::diag::LogLevel, char>
{
    constexpr auto parse(std::format_parse_context& context)
    {
        return context.begin();
    }

    auto format(my::diag::LogLevel level, format_context& context) const
    {
        using Level = my::diag::LogLevel;
        std::string_view str = "unknown";
        if (level == Level::Verbose)
        {
            str = "Verbose";
        }
        else if (level == Level::Debug)
        {
            str = "Debug";
        }
        else if (level == Level::Info)
        {
            str = "Info";
        }
        else if (level == Level::Warning)
        {
            str = "Warning";
        }
        else if (level == Level::Error)
        {
            str = "Error";
        }
        else if (level == Level::Critical)
        {
            str = "Critical";
        }

        return std::vformat_to(context.out(), "{}", std::make_format_args(str));
    };
};

namespace my::diag
{
    namespace
    {
        LoggerPtr s_internalLogger = rtti::createInstance<LoggerImpl>();
        LoggerPtr s_defaultLogger = s_internalLogger;

        class DefaultFormatter final : public LogMessageFormatter
        {
            MY_REFCOUNTED_CLASS(my::diag::DefaultFormatter, LogMessageFormatter)

            std::string formatMessage(const LogMessage& message) const override
            {
                constexpr size_t TimeStrLen = 128;
                char timeStr[TimeStrLen];
                strftime(timeStr, TimeStrLen, "%F %H:%M:%S", std::localtime(&message.timeStamp));

                const SourceInfo& src = message.source;

                std::string formattedMessage = std::format("{}({}):", src.filePath, src.line.value_or(0));
                formattedMessage.append(std::format("[{}]:{}", message.level, message.text));
                // if ()
                // {
                //     formattedMessage.append(std::format(" at {}({})", src.filePath, src.line.value_or(0)));
                // }


                // std::string formattedMessage = std::format("[{}][{}]{}", std::string_view{timeStr}, message.level, message.text);
                // if (const SourceInfo& src = message.source)
                // {
                //     formattedMessage.append(std::format(" at {}({})", src.filePath, src.line.value_or(0)));
                // }

                return formattedMessage;
            }
        };
    }  // namespace

    LoggerImpl::LoggerImpl()
    {
        m_defaultFormatter = rtti::createInstanceSingleton<DefaultFormatter>();
    }

    LoggerImpl::~LoggerImpl()
    {
        const std::lock_guard lock(m_mutex);
        for (auto& subscription : m_sinks)
        {
            subscription.resetLogger();
        }

        m_sinks.clear();
    }

    void LoggerImpl::setName(std::string name)
    {
        m_name = std::move(name);
    }

    const std::string& LoggerImpl::getName() const
    {
        return m_name;
    }

    void LoggerImpl::log(LogLevel level, SourceInfo sourceInfo, LogContextPtr context, std::string text)
    {
        static thread_local unsigned s_recursionCounter = 0;
        static thread_local std::vector<LogMessage> s_pendingMessages;

        // preventing logMessage from recursive call (ever subscriber can subsequently invoke logging):
        rtstack_scope;

        ++s_recursionCounter;
        scope_on_leave
        {
            MY_DEBUG_ASSERT(s_recursionCounter > 0);
            --s_recursionCounter;
        };

        LogMessage logMessage{
            .threadId = std::this_thread::get_id(),
            .timeStamp = std::time(nullptr),
            .level = level,
            .source = sourceInfo,
            .text = std::move(text),
            .context = context};

        if (s_recursionCounter > 1)
        {
            s_pendingMessages.emplace_back(std::move(logMessage));
            return;
        }

        //std::string defaultMessage;

        const std::shared_lock lock{m_mutex};

        for (LogSinkEntryImpl& sink : m_sinks)
        {
            std::string formattedMessage;
            const LogMessageFormatter* formatter = sink.getFormatter();
            if (!formatter)
            {
                formattedMessage = m_defaultFormatter->formatMessage(logMessage);
            }
            else
            {
                formattedMessage = formatter->formatMessage(logMessage);
            }

            sink.log(logMessage, formattedMessage);
        }
    }

    LogSinkEntry LoggerImpl::addSink(LogSinkPtr sink, LogMessageFormatterPtr customFormatter)
    {
        auto entry = std::make_unique<LogSinkEntryImpl>(*this, std::move(sink), std::move(customFormatter));
        m_sinks.push_back(*entry);

        return entry;
    }

    void LoggerImpl::releaseLogSink(LogSinkEntryImpl& sink)
    {
        const std::lock_guard lock(m_mutex);

        MY_DEBUG_ASSERT(m_sinks.contains(sink));
        m_sinks.removeElement(sink);
    }

    void LoggerImpl::addFilter([[maybe_unused]] LogFilterPtr filter)
    {
        MY_FATAL_FAILURE("addFilter not implemented");
    }

    void LoggerImpl::removeFilter([[maybe_unused]] const LogFilter& filer)
    {
        MY_FATAL_FAILURE("removeFilter not implemented");
    }

    void LoggerImpl::setDefaultFormatter(LogMessageFormatterPtr formatter)
    {
        m_defaultFormatter = std::move(formatter);

        if (!m_defaultFormatter)
        {
            m_defaultFormatter = rtti::createInstanceSingleton<DefaultFormatter>();
        }
    }

    LoggerPtr createLogger()
    {
        return rtti::createInstance<LoggerImpl>();
    }

    void setDefaultLogger(LoggerPtr logger, LoggerPtr* oldLogger)
    {
        if (oldLogger && s_defaultLogger.get() != s_internalLogger.get())
        {
            *oldLogger = s_defaultLogger;
        }

        if (!logger)
        {
            s_defaultLogger = s_internalLogger;
        }
        else
        {
            s_defaultLogger = std::move(logger);
        }
    }

    bool hasDefaultLogger()
    {
        return s_defaultLogger != nullptr;
    }

    Logger& getDefaultLogger()
    {
        MY_FATAL(s_defaultLogger);
        return *s_defaultLogger;
    }

#if 0
    Logger::SubscriptionHandle LoggerImpl::subscribeImpl(ILogSubscriber::Ptr subscriber, ILogMessageFilter::Ptr filter)
    {
        const std::lock_guard lock(m_mutex);

        m_subscribers.emplace_back(std::move(subscriber), std::move(filter), ++m_subscriberId);
        return makeSubscriptionHandle(shared_from_this(), m_subscribers.back().id);
    }

    void LoggerImpl::releaseSubscriptionImpl(uint32_t subscriptionId)
    {
        if (subscriptionId == 0)
        {
            return;
        }

        const std::lock_guard lock(m_mutex);

        auto iter = std::find_if(m_subscribers.begin(), m_subscribers.end(), [subscriptionId](const SubscriberEntry& entry)
        {
            return entry.id == subscriptionId;
        });
        MY_DEBUG_ASSERT(iter != m_subscribers.end());

        if (iter != m_subscribers.end())
        {
            m_subscribers.erase(iter);
        }
    }

    void LoggerImpl::setFilterImpl(const SubscriptionHandle& handle, ILogMessageFilter::Ptr)
    {
        const std::lock_guard lock(m_mutex);
    }

    void LoggerImpl::logMessage(LogLevel criticality, std::vector<std::string> tags, SourceInfo sourceInfo, std::string text)
    {
        static thread_local unsigned recursionCounter = 0;
        static thread_local std::vector<LoggerMessage> pendingMessages;

        // preventing logMessage from recursive call (ever subscriber can subsequently invoke logging):
        ++recursionCounter;
        scope_on_leave
        {
            NAU_FATAL(recursionCounter > 0);
            --recursionCounter;
        };

        LoggerMessage message{
            .index = m_messageIndex.fetch_add(1, std::memory_order_relaxed),
            .time = std::time(nullptr),
            .level = criticality,
            .tags = std::move(tags),
            .source = sourceInfo,
            .data = std::move(text)};

        if (recursionCounter > 1)
        {
            pendingMessages.emplace_back(std::move(message));
            return;
        }

        const std::shared_lock lock{m_mutex};

        for (auto& subscriber : m_subscribers)
        {
            subscriber(message);
        }

        while (!pendingMessages.empty())
        {
            auto messages = std::move(pendingMessages);
            pendingMessages.clear();

            for (const auto& message : messages)
            {
                for (auto& subscriber : m_subscribers)
                {
                    subscriber(message);
                }
            }
        }
    }

    Logger::SubscriptionHandle::SubscriptionHandle(Logger::Ptr&& logger, uint32_t id) :
        m_logger(std::move(logger)),
        m_id(id)
    {
    }

    Logger::SubscriptionHandle::SubscriptionHandle(SubscriptionHandle&& other) :
        m_logger(std::move(other.m_logger)),
        m_id(std::exchange(other.m_id, 0))
    {
        MY_DEBUG_ASSERT(other.m_logger.expired());
    }

    Logger::SubscriptionHandle::~SubscriptionHandle()
    {
        release();
    }

    Logger::SubscriptionHandle& Logger::SubscriptionHandle::operator=(SubscriptionHandle&& other)
    {
        release();

        m_logger = std::move(other.m_logger);
        m_id = std::exchange(other.m_id, 0);

        MY_DEBUG_ASSERT(other.m_logger.expired());

        return *this;
    }

    Logger::SubscriptionHandle::operator bool() const
    {
        return !m_logger.expired() && m_id > 0;
    }

    void Logger::SubscriptionHandle::release()
    {
        if (auto logger = m_logger.lock())
        {
            const auto id = std::exchange(m_id, 0);
            m_logger.reset();

            static_cast<LoggerImpl*>(logger.get())->releaseSubscriptionImpl(id);
        }
    }

    namespace
    {
        Logger::Ptr& getLoggerRef()
        {
            static Logger::Ptr s_logger;
            return (s_logger);
        }
    }  // namespace

    Logger::Ptr createLogger()
    {
        return std::make_shared<LoggerImpl>();
    }

    void setLogger(Logger::Ptr&& logger)
    {
        MY_DEBUG_ASSERT(!logger || !getLoggerRef(), "Logger instance already set");
        getLoggerRef() = std::move(logger);
    }

    Logger& getLogger()
    {
        auto& logger = getLoggerRef();
        NAU_FATAL(logger);
        return *logger;
    }

    bool hasLogger()
    {
        return static_cast<bool>(getLoggerRef());
    }

    Logger::SubscriptionHandle Logger::makeSubscriptionHandle(Logger::Ptr&& logger, uint32_t id)
    {
        return {std::move(logger), id};
    }
#endif
}  // namespace my::diag
