// #my_engine_source_file
#pragma once

#include <atomic>
#include <concepts>
#include <string_view>
#include <type_traits>

#include "my/diag/check.h"
#include "my/kernel/kernel_config.h"
#include "my/rtti/rtti_impl.h"
#include "my/rtti/weak_ptr.h"
#include "my/serialization/runtime_value.h"
#include "my/threading/lock_guard.h"
#include "my/utils/preprocessor.h"
#include "my/utils/scope_guard.h"

namespace my
{
    /**
     */
    struct MY_ABSTRACT_TYPE IRuntimeValueChangesHandler : virtual IRefCounted
    {
        MY_INTERFACE(my::IRuntimeValueChangesHandler, IRefCounted)

        virtual void onValueChanged(const RuntimeValue& target, std::string_view childKey) = 0;
    };

    /**
     */
    struct MY_ABSTRACT_TYPE IRuntimeValueEvents : virtual IRefCounted
    {
        MY_INTERFACE(my::IRuntimeValueEvents, IRefCounted)

        class [[nodiscard]] SubscriptionHandle
        {
        public:
            SubscriptionHandle() = default;

            SubscriptionHandle(my::Ptr<IRuntimeValueEvents> value, uint32_t uid) :
                m_valueRef(std::move(value)),
                m_uid(uid)
            {
            }

            SubscriptionHandle(const SubscriptionHandle&) = delete;

            SubscriptionHandle(SubscriptionHandle&& other) :
                m_valueRef(std::move(other.m_valueRef)),
                m_uid(std::exchange(other.m_uid, 0))
            {
            }

            ~SubscriptionHandle()
            {
                reset();
            }

            SubscriptionHandle& operator=(const SubscriptionHandle&) = delete;

            SubscriptionHandle& operator=(SubscriptionHandle&& other)
            {
                reset();

                m_valueRef = std::move(other.m_valueRef);
                m_uid = std::exchange(other.m_uid, 0);

                return *this;
            }

            SubscriptionHandle& operator=(std::nullptr_t)
            {
                reset();
                return *this;
            }

            void reset()
            {
#if MY_DEBUG_CHECK_ENABLED
                scope_on_leave
                {
                    MY_DEBUG_CHECK(!m_valueRef);
                    MY_DEBUG_CHECK(m_uid == 0);
                };
#endif
                if(!m_valueRef)
                {
                    m_uid = 0;
                    return;
                }

                auto valueRef = std::move(m_valueRef);
                if(const auto uid = std::exchange(m_uid, 0); uid > 0)
                {
                    if(const auto value = valueRef.lock())
                    {
                        value->unsubscribe(uid);
                    }
                }
            }

            explicit operator bool() const
            {
                return m_uid != 0;
            }

        private:
            my::WeakPtr<IRuntimeValueEvents> m_valueRef;
            uint32_t m_uid = 0;
        };

        virtual SubscriptionHandle subscribeOnChanges(my::Ptr<IRuntimeValueChangesHandler>) = 0;

        template <std::invocable<const RuntimeValue&, std::string_view> F>
        SubscriptionHandle subscribeOnChanges(F&& functorHandler);

    protected:
        virtual void unsubscribe(uint32_t id) = 0;
    };

    /**
     */
    struct MY_ABSTRACT_TYPE IRuntimeValueEventsSource : virtual IRefCounted
    {
        MY_INTERFACE(my::IRuntimeValueEventsSource, IRefCounted)

        virtual void notifyChanged(const RuntimeValue* source = nullptr) = 0;
    };

}  // namespace my

namespace my::ser_detail
{
    template <std::invocable<const RuntimeValue&, std::string_view> F>
    class ChangesHandlerFunctorWrapper final : public my::IRuntimeValueChangesHandler
    {
        MY_REFCOUNTED_CLASS(my::ser_detail::ChangesHandlerFunctorWrapper<F>, my::IRuntimeValueChangesHandler)

    public:
        ChangesHandlerFunctorWrapper(F&& handler) :
            m_handler(std::move(handler))
        {
        }

    private:
        void onValueChanged(const RuntimeValue& target, std::string_view childKey) override
        {
            m_handler(target, childKey);
        }

        F m_handler;
    };

    template <typename F>
    ChangesHandlerFunctorWrapper(F) -> ChangesHandlerFunctorWrapper<F>;

    struct ValueChangesScopeHelper
    {
        IRuntimeValueEventsSource& value;

        ValueChangesScopeHelper(IRuntimeValueEventsSource& inValue) :
            value(inValue)
        {
        }

        ~ValueChangesScopeHelper()
        {
            value.notifyChanged();
        }
    };

}  // namespace my::ser_detail

namespace my
{
    template <std::invocable<const RuntimeValue&, std::string_view> F>
    inline IRuntimeValueEvents::SubscriptionHandle IRuntimeValueEvents::subscribeOnChanges(F&& functorHandler)
    {
        using Handler = ser_detail::ChangesHandlerFunctorWrapper<F>;

        auto handlerObject = rtti::createInstance<Handler, IRuntimeValueChangesHandler>(std::move(functorHandler));
        return subscribeOnChanges(std::move(handlerObject));
    }
}  // namespace my

// clang-format off
#define value_changes_scope const ::my::ser_detail::ValueChangesScopeHelper ANONYMOUS_VAR(changesScope__) {*this}
// clang-format on
