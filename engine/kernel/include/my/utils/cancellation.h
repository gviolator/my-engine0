// #my_engine_source_header


#pragma once

#include <optional>
#include <chrono>
#include <memory.h>

#include "my/kernel/kernel_config.h"

namespace my::kernel_detail
{
    class CancellationState;
    class ExpirationState;

}  // namespace my::kernel_detail

namespace my
{

    /**
     */
    class MY_KERNEL_EXPORT [[nodiscard]] CancellationSubscription final
    {
    public:
        ~CancellationSubscription();

        CancellationSubscription();

        CancellationSubscription(CancellationSubscription&&);

        CancellationSubscription(const CancellationSubscription&) = delete;

        CancellationSubscription& operator= (CancellationSubscription&&);

        CancellationSubscription& operator= (const CancellationSubscription&) = delete;

        CancellationSubscription& operator= (std::nullptr_t);

        explicit operator bool () const;

        void reset();

    private:
        CancellationSubscription(std::shared_ptr<kernel_detail::CancellationState>, uintptr_t);

        std::shared_ptr<kernel_detail::CancellationState> m_cancellation;
        uintptr_t m_subscriptionHandle = 0;

        friend class Cancellation;
    };

    /**
     */
    class MY_KERNEL_EXPORT [[nodiscard]] ExpirationSubscription final
    {
    public:
        ~ExpirationSubscription();

        ExpirationSubscription();

        ExpirationSubscription(ExpirationSubscription&&);

        ExpirationSubscription(const ExpirationSubscription&) = delete;

        ExpirationSubscription& operator= (ExpirationSubscription&&);

        ExpirationSubscription& operator= (const ExpirationSubscription&) = delete;

        explicit operator bool () const;

        void reset();

    private:
        ExpirationSubscription(std::shared_ptr<kernel_detail::ExpirationState>, uintptr_t);

        std::shared_ptr<kernel_detail::ExpirationState> m_expiration;
        uintptr_t m_subscriptionHandle = 0;

        friend class Expiration;
    };

    /**
     */
    class MY_KERNEL_EXPORT Cancellation final
    {
    public:
        static Cancellation none();

        ~Cancellation();

        Cancellation();

        Cancellation(std::nullptr_t)
        {
        }

        Cancellation(Cancellation&&);

        Cancellation(const Cancellation&);

        Cancellation& operator= (Cancellation&&);

        Cancellation& operator= (const Cancellation&);

        bool isCancelled() const;

        bool isEternal() const;

        CancellationSubscription subscribe(void (*callback)(void*), void* callbackData);

    private:
        Cancellation(std::shared_ptr<kernel_detail::CancellationState> token);

        std::shared_ptr<kernel_detail::CancellationState> m_cancellation;

        friend class CancellationSource;
        friend class Expiration;
    };

    /**
     */
    class MY_KERNEL_EXPORT CancellationSource final
    {
    public:
        ~CancellationSource();

        CancellationSource();

        CancellationSource(std::nullptr_t);

        CancellationSource(CancellationSource&&);

        CancellationSource(const CancellationSource&) = delete;

        CancellationSource& operator= (CancellationSource&&);

        CancellationSource& operator= (const CancellationSource&) = delete;

        explicit operator bool () const;

        Cancellation getCancellation();

        bool isCancelled() const;

        void cancel();

        template <typename Rep, typename Period>
        void setTimeout(std::chrono::duration<Rep, Period> timeout)
        {
            using namespace std::chrono;

            setTimeoutInternal(duration_cast<milliseconds>(timeout));
        }

    private:
        void SetTimeoutInternal(std::chrono::milliseconds);

        std::shared_ptr<kernel_detail::CancellationState> m_cancellation;
    };

    /**
     */
    class MY_KERNEL_EXPORT Expiration
    {
    public:
        static Expiration never();

        Expiration(Cancellation cancellation, std::chrono::milliseconds timeout);

        Expiration(Cancellation cancellation);

        Expiration(std::chrono::milliseconds timeout);

        bool isExpired() const;

        bool isEternal() const;

        ExpirationSubscription subscribe(void (*)(void*), void*);

        std::optional<std::chrono::milliseconds> getTimeout() const;

    private:
        Expiration();

        std::shared_ptr<kernel_detail::ExpirationState> m_expiration;
    };

}  // namespace my
