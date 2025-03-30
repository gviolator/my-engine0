// #my_engine_source_file
#include "logger_impl.h"

#include "my/threading/lock_guard.h"
#include "my/utils/scope_guard.h"

namespace my::diag
{
    LoggerImpl::~LoggerImpl()
    {
        lock_(m_mutex);
        for (auto& subscription : m_subscriptions)
        {
            subscription.resetLogger();
        }

        m_subscriptions.clear();
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
        ++s_recursionCounter;
        scope_on_leave
        {
            MY_DEBUG_CHECK(s_recursionCounter > 0);
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

        const std::shared_lock lock{m_mutex};
        for (auto& subscription : m_subscriptions)
        {
            subscription.log(logMessage);
        }
    }

    LogSubscription LoggerImpl::subscribe(LogSubscriberPtr subscriber)
    {
        auto subscription = std::make_unique<LogSubscriptionImpl>(*this, std::move(subscriber));
        m_subscriptions.push_back(*subscription);

        return subscription;
    }

    void LoggerImpl::releaseSubscription(LogSubscriptionImpl& subscription)
    {
        lock_(m_mutex);

        MY_DEBUG_CHECK(m_subscriptions.contains(subscription));
        m_subscriptions.removeElement(subscription);
    }

    void LoggerImpl::addFilter([[maybe_unused]] LogFilterPtr filter)
    {
        MY_FATAL_FAILURE("addFilter not implemented");
    }

    void LoggerImpl::removeFilter([[maybe_unused]] const LogFilter& filer)
    {
        MY_FATAL_FAILURE("removeFilter not implemented");
    }

#if 0
    Logger::SubscriptionHandle LoggerImpl::subscribeImpl(ILogSubscriber::Ptr subscriber, ILogMessageFilter::Ptr filter)
    {
        lock_(m_mutex);

        m_subscribers.emplace_back(std::move(subscriber), std::move(filter), ++m_subscriberId);
        return makeSubscriptionHandle(shared_from_this(), m_subscribers.back().id);
    }

    void LoggerImpl::releaseSubscriptionImpl(uint32_t subscriptionId)
    {
        if (subscriptionId == 0)
        {
            return;
        }

        lock_(m_mutex);

        auto iter = std::find_if(m_subscribers.begin(), m_subscribers.end(), [subscriptionId](const SubscriberEntry& entry)
        {
            return entry.id == subscriptionId;
        });
        MY_DEBUG_CHECK(iter != m_subscribers.end());

        if (iter != m_subscribers.end())
        {
            m_subscribers.erase(iter);
        }
    }

    void LoggerImpl::setFilterImpl(const SubscriptionHandle& handle, ILogMessageFilter::Ptr)
    {
        lock_(m_mutex);
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
        MY_DEBUG_CHECK(other.m_logger.expired());
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

        MY_DEBUG_CHECK(other.m_logger.expired());

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
        MY_DEBUG_CHECK(!logger || !getLoggerRef(), "Logger instance already set");
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
