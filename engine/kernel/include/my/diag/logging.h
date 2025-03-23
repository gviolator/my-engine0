// #my_engine_source_header
#pragma once

#include <string_view>

#include "my/diag/logging_base.h"
#include "my/utils/format_helper.h"
#include "my/utils/functor.h"

namespace my::diag
{
    // template <typename... Args>
    // void log(LogLevel level, SourceInfo source, const LogContext* context, std::string_view message, const Args&... args)
    // {
    // }

    // template <typename... Args>
    // void log(LogLevel level, SourceInfo source, const LogContext* context, std::wstring_view message, const Args&... args)
    // {
    // }

    MY_KERNEL_EXPORT LoggerPtr createLogger();

    MY_KERNEL_EXPORT void setDefaultLogger(LoggerPtr logger, LoggerPtr* oldLogger = nullptr);

    MY_KERNEL_EXPORT Logger& getDefaultLogger();
}  // namespace my::diag

namespace my::diag_detail
{
    struct InplaceLogData
    {
        diag::Logger* const logger = nullptr;
        diag::LogContextPtr context;
        diag::LogLevel level;
        diag::SourceInfo sourceInfo;

        InplaceLogData(diag::Logger* inLogger, diag::LogContextPtr inContext, diag::LogLevel inLevel, diag::SourceInfo inSourceInfo) :
            logger(inLogger),
            context(std::move(inContext)),
            level(inLevel),
            sourceInfo(inSourceInfo)
        {
        }

        // InplaceLogData(diag::Logger* inLogger, diag::LogLevel inLevel, diag::SourceInfo inSourceInfo) :
        //     logger(inLogger),
        //     level(inLevel),
        //     sourceInfo(inSourceInfo)
        // {
        // }

        template <typename Arg, typename... MoreArgs>
        void operator()(std::format_string<Arg, MoreArgs...> str, Arg&& arg0, MoreArgs&&... args)
        {
            std::string message = std::format(std::move(str), std::forward<Arg>(arg0), std::forward<MoreArgs>(args)...);
            getLogger().log(level, sourceInfo, std::move(context), std::move(message));
        }

        template <typename Arg, typename... MoreArgs>
        void operator()(std::wformat_string<Arg, MoreArgs...> wstr, Arg&& arg0, MoreArgs&&... args)
        {
            const std::wstring wcsMessage = std::format(std::move(wstr), std::forward<Arg>(arg0), std::forward<MoreArgs>(args)...);
            std::string message = strings::wstringToUtf8(wcsMessage);
            getLogger().log(level, sourceInfo, std::move(context), std::move(message));
        }

        void operator()(std::string message)
        {
            getLogger().log(level, sourceInfo, std::move(context), std::move(message));
        }

        void operator()(const std::wstring& wcsMessage)
        {
            std::string message = strings::wstringToUtf8(wcsMessage);
            getLogger().log(level, sourceInfo, std::move(context), std::move(message));
        }

    private:
        diag::Logger& getLogger() const
        {
            return logger ? *logger : diag::getDefaultLogger();
        }
    };
}  // namespace my::diag_detail

#if 0
namespace my::diag_detail
{
    template <typename, typename>
    struct CompatibleUniquePtr : std::false_type
    {
    };

    template <typename T, typename U>
    struct CompatibleUniquePtr<std::shared_ptr<T>, U> : std::bool_constant<std::is_assignable_v<U&, T&>>
    {
    };

}  // namespace my::diag_detail

namespace my::diag
{
  

    template <typename T>
    constexpr bool IsInvocableLogSubscriber = std::is_invocable_v<T, const LoggerMessage&>;

    template <typename T>
    constexpr bool IsInvocableLogMessageFilter = std::is_invocable_r_v<bool, T, const LoggerMessage&>;

    template <typename T>
    concept LogSubscriberConcept = IsInvocableLogSubscriber<T> || diag_detail::CompatibleUniquePtr<T, ILogSubscriber>::value;

    template <typename T>
    concept LogFilterConcept = IsInvocableLogMessageFilter<T> || diag_detail::CompatibleUniquePtr<T, ILogMessageFilter>::value || std::is_null_pointer_v<T>;

    /**
     */
    class MY_ABSTRACT_TYPE Logger
    {
    public:
        using Ptr = std::shared_ptr<Logger>;

        class MY_KERNEL_EXPORT [[nodiscard]] SubscriptionHandle
        {
        public:
            SubscriptionHandle() = default;
            SubscriptionHandle(SubscriptionHandle&&);
            SubscriptionHandle(const SubscriptionHandle&) = delete;
            ~SubscriptionHandle();

            SubscriptionHandle& operator=(SubscriptionHandle&&);
            SubscriptionHandle& operator=(const SubscriptionHandle&) = delete;
            inline SubscriptionHandle& operator=(std::nullptr_t)
            {
                release();
                return *this;
            }

            explicit operator bool() const;

            void release();

        private:
            SubscriptionHandle(Logger::Ptr&&, uint32_t);

            std::weak_ptr<Logger> m_logger;
            uint32_t m_id = 0;

            friend Logger;
        };

        virtual ~Logger() = default;

        virtual void logMessage(LogLevel criticality, std::vector<std::string> tags, SourceInfo sourceInfo, std::string text) = 0;

        template <LogSubscriberConcept TSubscriber, LogFilterConcept TFilter = std::nullptr_t>
        SubscriptionHandle subscribe(TSubscriber subscriber, TFilter filter = nullptr);

        template <LogFilterConcept TFilter>
        void setFilter(const SubscriptionHandle& handle, TFilter filter);

        void resetFilter(const SubscriptionHandle& handle);

    protected:
        virtual SubscriptionHandle subscribeImpl(ILogSubscriber::Ptr subscriber, ILogMessageFilter::Ptr = nullptr) = 0;

        virtual void releaseSubscriptionImpl(uint32_t subscriptionId) = 0;

        virtual void setFilterImpl(const SubscriptionHandle& handle, ILogMessageFilter::Ptr) = 0;

        static class SubscriptionHandle makeSubscriptionHandle(Logger::Ptr&&, uint32_t);

    private:
        template <LogFilterConcept TFilter>
        static ILogMessageFilter::Ptr makeLogMessageFilterPtr(TFilter filter);
    };

    MY_KERNEL_EXPORT Logger::Ptr createLogger();

    MY_KERNEL_EXPORT void setLogger(Logger::Ptr&&);

    MY_KERNEL_EXPORT Logger& getLogger();

    MY_KERNEL_EXPORT bool hasLogger();

}  // namespace my::diag

namespace my::diag_detail
{
    template <typename F>
    requires(diag::IsInvocableLogSubscriber<F>)
    class FunctionalLogSubscriber final : public diag::ILogSubscriber
    {
    public:
        FunctionalLogSubscriber(F&& callback) :
            m_callback(std::move(callback))
        {
        }

    private:
        void processMessage(const diag::LoggerMessage& message) override
        {
            // assert m_callback
            m_callback(message);
        }

        my::Functor<void(const diag::LoggerMessage&)> m_callback;
    };

    template <typename F>
    requires(diag::IsInvocableLogMessageFilter<F>)
    class FunctionalMessageFilter final : public diag::ILogMessageFilter
    {
    public:
        FunctionalMessageFilter(F&& callback) :
            m_callback(std::move(callback))
        {
        }

    private:
        bool acceptMessage(const diag::LoggerMessage& message) override
        {
            // assert m_callback
            return m_callback(message);
        }

        my::Functor<bool(const diag::LoggerMessage&)> m_callback;
    };

    struct InplaceLogData
    {
        diag::LogLevel level;
        diag::SourceInfo sourceInfo;

        InplaceLogData(diag::LogLevel inLevel, diag::SourceInfo inSourceInfo) :
            level(inLevel),
            sourceInfo(inSourceInfo)
        {
        }

        template <typename S, typename... Args>
        void operator()(std::vector<std::string> tags, S&& formatStr, Args&&... args)
        {
            if constexpr (sizeof...(Args) > 0)
            {
                auto message = std::format(formatStr, std::forward<Args>(args)...);
                diag::getLogger().logMessage(level, std::move(tags), sourceInfo, std::move(message));
            }
            else
            {
                auto message = std::format(formatStr);
                diag::getLogger().logMessage(level, std::move(tags), sourceInfo, std::move(message));
            }
        }

        template <typename S, typename... Args>
        void operator()(S&& formatStr, Args&&... args)
        {
            operator()(std::vector<std::string>{}, std::forward<S>(formatStr), std::forward<Args>(args)...);
        }
    };

}  // namespace my::diag_detail

namespace my::diag
{
    template <LogFilterConcept TFilter>
    ILogMessageFilter::Ptr Logger::makeLogMessageFilterPtr(TFilter filter)
    {
        if constexpr (IsInvocableLogMessageFilter<TFilter>)
        {
            using MessageFilter = diag_detail::FunctionalMessageFilter<TFilter>;
            return std::make_shared<MessageFilter>(std::move(filter));
        }
        else if constexpr (std::is_null_pointer_v<TFilter>)
        {
            return nullptr;
        }
        else
        {
            return std::move(filter);
        }
    }

    template <LogSubscriberConcept TSubscriber, LogFilterConcept TFilter>
    Logger::SubscriptionHandle Logger::subscribe(TSubscriber subscriber, TFilter filter)
    {
        ILogSubscriber::Ptr subscriberPtr;
        if constexpr (IsInvocableLogSubscriber<TSubscriber>)
        {
            using Subscriber = diag_detail::FunctionalLogSubscriber<TSubscriber>;
            subscriberPtr = std::make_shared<Subscriber>(std::move(subscriber));
        }
        else
        {
            subscriberPtr = subscriber;
        }

        auto filterPtr = makeLogMessageFilterPtr(filter);

        return subscribeImpl(subscriberPtr, filterPtr);
    }

    template <LogFilterConcept TFilter>
    void Logger::setFilter(const SubscriptionHandle& handle, TFilter filter)
    {
        setFilterImpl(handle, makeLogMessageFilterPtr(std::move(filter)));
    }

    inline void Logger::resetFilter(const SubscriptionHandle& handle)
    {
        setFilterImpl(handle, nullptr);
    }

}  // namespace my::diag

// clang-format off
#endif



#define mylog_verbose ::my::diag_detail::InplaceLogData{ nullptr, nullptr, ::my::diag::LogLevel::Verbose, MY_INLINED_SOURCE_INFO }
#define mylog_debug ::my::diag_detail::InplaceLogData{ nullptr, nullptr, ::my::diag::LogLevel::Debug, MY_INLINED_SOURCE_INFO }
#define mylog_info ::my::diag_detail::InplaceLogData{ nullptr, nullptr, ::my::diag::LogLevel::Info, MY_INLINED_SOURCE_INFO }
#define mylog_warn ::my::diag_detail::InplaceLogData{ nullptr, nullptr, ::my::diag::LogLevel::Warning, MY_INLINED_SOURCE_INFO }
#define mylog_err ::my::diag_detail::InplaceLogData{ nullptr, nullptr, ::my::diag::LogLevel::Error, MY_INLINED_SOURCE_INFO }
#define mylog_crit ::my::diag_detail::InplaceLogData{ nullptr, nullptr, ::my::diag::LogLevel::Critical, MY_INLINED_SOURCE_INFO }

#define mylog mylog_verbose


// #define MY_LOG_INFO \
//     ::my::diag_detail::InplaceLogData{ ::my::diag::LogLevel::Info, MY_INLINED_SOURCE_INFO }

// #define MY_LOG_DEBUG \
//     ::my::diag_detail::InplaceLogData{ ::my::diag::LogLevel::Debug, MY_INLINED_SOURCE_INFO }

// #define MY_LOG_WARNING \
//     ::my::diag_detail::InplaceLogData{ ::my::diag::LogLevel::Warning, MY_INLINED_SOURCE_INFO }

// #define MY_LOG_ERROR \
//     ::my::diag_detail::InplaceLogData{ ::my::diag::LogLevel::Error, MY_INLINED_SOURCE_INFO }

// #define MY_LOG_CRITICAL \
//     ::my::diag_detail::InplaceLogData{ ::my::diag::LogLevel::Critical, MY_INLINED_SOURCE_INFO }

// #ifdef  MY_VERBOSE_LOG
// #define MY_LOG_VERBOSE \
//     ::my::diag_detail::InplaceLogData{ ::my::diag::LogLevel::Verbose, MY_INLINED_SOURCE_INFO }
// #else
// #define MY_LOG_VERBOSE(tags, formatStr, ...) \
//     do {        }                             \
//     while(false)
// #endif

// #define MY_LOG \
//     ::my::diag_detail::InplaceLogData{ ::my::diag::LogLevel::Debug, MY_INLINED_SOURCE_INFO }

// clang-format on

// #define MY_CONDITION_LOG(condition, criticality, tags, formatStr, ...) \
//     {                                                                  \
//         if (!!(condition))                                             \
//         {                                                              \
//             MY_LOG_MESSAGE(criticality)                                \
//             (tags, formatStr, ##__VA_ARGS__);                          \
//         }                                                              \
//     }

// #define MY_ENSURE_LOG(criticality, tags, formatStr, ...)                   \
//     {                                                                      \
//         static std::atomic<bool> s_wasTrigered = false;                    \
//         bool value = s_wasTrigered.load(std::memory_order_relaxed);        \
//         if (!value && !s_wasTrigered.compare_exchange_strong(value, true)) \
//         {                                                                  \
//             MY_LOG_MESSAGE(criticality)                                    \
//             (tags, formatStr, ##__VA_ARGS__);                              \
//         }                                                                  \
//     }
