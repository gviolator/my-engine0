// #my_engine_source_file

#pragma once

#include <concepts>

#include "my/diag/check.h"
#include "my/rtti/weak_ptr.h"
#include "my/serialization/runtime_value.h"
#include "my/serialization/runtime_value_events.h"

namespace my::ser_detail
{
    /**
     */
    class RuntimeValueEventsBase : public virtual IRuntimeValueEvents,
                                   public virtual IRuntimeValueEventsSource
    {
        MY_INTERFACE(my::ser_detail::RuntimeValueEventsBase, IRuntimeValueEvents, IRuntimeValueEventsSource)

        RuntimeValueEventsBase() = default;

    public:
        SubscriptionHandle subscribeOnChanges(my::Ptr<IRuntimeValueChangesHandler> handler) final
        {
            MY_DEBUG_FATAL(handler);
#if MY_DEBUG_CHECK_ENABLED
            MY_DEBUG_CHECK(m_concurrentCheckFlag.exchange(true, std::memory_order_acquire) == false);
            scope_on_leave
            {
                m_concurrentCheckFlag.store(false, std::memory_order_release);
            };
#endif

            const auto& entry = m_changeHandlers.emplace_back(std::move(handler), ++m_id);
            return SubscriptionHandle{this, std::get<1>(entry)};
        }

        template <std::invocable<const RuntimeValue&, std::string_view> F>
        SubscriptionHandle subscribeOnChanges(F&& functorHandler)
        {
            return IRuntimeValueEvents::subscribeOnChanges(std::forward<F>(functorHandler));
        }


    protected:
        void notifyChanged(const RuntimeValue* source = nullptr) final
        {
            const std::string_view fieldName = source ? findFieldName(*source) : std::string_view{};
            const RuntimeValue& thisAsRuntimeValue = this->as<const RuntimeValue&>();

            notifyHandlers(thisAsRuntimeValue, fieldName);

            if(auto parent = getParent())
            {
                if(auto* const parentEvents = parent->as<IRuntimeValueEventsSource*>())
                {
                    parentEvents->notifyChanged(&thisAsRuntimeValue);
                }
            }
        }

        virtual std::string_view findFieldName([[maybe_unused]] const RuntimeValue& value) const
        {
            return {};
        }

        virtual RuntimeValue* getParent() const
        {
            return nullptr;
        }

        virtual void onThisValueChanged([[maybe_unused]] std::string_view key)
        {}

    private:
        using ChangesHandlerEntry = std::tuple<my::Ptr<IRuntimeValueChangesHandler>, uint32_t>;

        void notifyHandlers(const RuntimeValue& thisAsRuntimeValue, std::string_view childKey)
        {
#if MY_DEBUG_CHECK_ENABLED
            MY_DEBUG_CHECK(m_concurrentCheckFlag.exchange(true, std::memory_order_acquire) == false);
            scope_on_leave
            {
                m_concurrentCheckFlag.store(false, std::memory_order_release);
            };
#endif

            onThisValueChanged(childKey);

            for(auto& handlerEntry : m_changeHandlers)
            {
                auto& handler = std::get<0>(handlerEntry);
                handler->onValueChanged(thisAsRuntimeValue, childKey);
            }
        }

        void unsubscribe(uint32_t id) final
        {
#if MY_DEBUG_CHECK_ENABLED
            MY_DEBUG_CHECK(m_concurrentCheckFlag.exchange(true, std::memory_order_acquire) == false);
            scope_on_leave
            {
                m_concurrentCheckFlag.store(false, std::memory_order_release);
            };
#endif
            auto entry = std::find_if(m_changeHandlers.begin(), m_changeHandlers.end(), [id](const ChangesHandlerEntry& entry)
            {
                return std::get<1>(entry) == id;
            });

            if(entry != m_changeHandlers.end())
            {
                m_changeHandlers.erase(entry);
            }
        }

        std::vector<ChangesHandlerEntry> m_changeHandlers;
        uint32_t m_id = 0;

#if MY_DEBUG_CHECK_ENABLED
        mutable std::atomic<bool> m_concurrentCheckFlag{false};
#endif
    };

    /**
     */

    class ParentMutabilityGuard final : public virtual IRefCounted
    {
        MY_REFCOUNTED_CLASS(my::ser_detail::ParentMutabilityGuard, IRefCounted)

    public:
        const RuntimeValuePtr& getParent() const
        {
            return m_parent;
        }

        ParentMutabilityGuard(RuntimeValuePtr parent) :
            m_parent(std::move(parent))
        {
        }

     private:
        const RuntimeValuePtr m_parent;
    };

    /**
     */
    class NativeChildValue
    {
        MY_TYPEID(my::ser_detail::NativeChildValue)

    public:
        void setParent(my::Ptr<ParentMutabilityGuard>&& parentGuard)
        {
            MY_DEBUG_CHECK(!m_parentGuard);
            m_parentGuard = std::move(parentGuard);
        }

    private:
        RuntimeValue* getParentObject() const
        {
            return m_parentGuard ? m_parentGuard->getParent().get() : nullptr;
        }

        my::Ptr<ParentMutabilityGuard> m_parentGuard;

        template <std::derived_from<RuntimeValue> T>
        friend class NativeRuntimeValueBase;

        template <std::derived_from<RuntimeValue> T>
        friend class NativePrimitiveRuntimeValueBase;
    };

    struct MY_ABSTRACT_TYPE NativeParentValue
    {
        MY_TYPEID(my::ser_detail::NativeParentValue)

        virtual my::Ptr<ParentMutabilityGuard> getThisMutabilityGuard() const = 0;
    };

    /**
     */
    template <std::derived_from<RuntimeValue> T>
    class NativePrimitiveRuntimeValueBase : public T,
                                            public NativeChildValue,
                                            public RuntimeValueEventsBase
    {
        MY_INTERFACE(my::ser_detail::NativePrimitiveRuntimeValueBase<T>, T, NativeChildValue, RuntimeValueEventsBase)

    private:
        RuntimeValue* getParent() const final
        {
            return this->getParentObject();
        }
    };

    /**
     */
    template <std::derived_from<RuntimeValue> T>
    class NativeRuntimeValueBase : public T,
                                   public NativeChildValue,
                                   public NativeParentValue,
                                   public RuntimeValueEventsBase
    {
        MY_INTERFACE(my::ser_detail::NativeRuntimeValueBase<T>, T, NativeChildValue, NativeParentValue, RuntimeValueEventsBase)

    protected:
        template <std::derived_from<RuntimeValue> U>
        my::Ptr<U>& makeChildValue(my::Ptr<U>& value) const
        {
            setThisAsParent(value);
            return (value);
        }

        template <std::derived_from<RuntimeValue> U>
        my::Ptr<U> makeChildValue(my::Ptr<U>&& value) const
        {
            setThisAsParent(value);
            return value;
        }

        bool hasChildren() const
        {
            return !m_mutabilityGuardRef.isDead();
        }

        template <std::derived_from<RuntimeValue> U>
        void setThisAsParent(my::Ptr<U>& value) const
        {
            MY_DEBUG_CHECK(value);
            if(NativeChildValue* const childValue = value->template as<NativeChildValue*>())
            {
                childValue->setParent(getThisMutabilityGuard());
            }
        }

         //template<typename U>
         //decltype(auto) getMutableThis(const U* self)
         //{
         //    if constexpr (!U::IsMutable && !U::IsReference)
         //    {
         //        using MutableThis = std::remove_const_t<U>;
         //        return const_cast<MutableThis>(self);
         //    }

         //    return self;
         //}

    private:
        RuntimeValue* getParent() const final
        {
            return this->getParentObject();
        }

        my::Ptr<ParentMutabilityGuard> getThisMutabilityGuard() const final
        {
            auto guard = m_mutabilityGuardRef.lock();
            if(!guard)
            {
                RuntimeValuePtr mutableThis = const_cast<NativeRuntimeValueBase<T>*>(this);
                m_mutabilityGuardRef.reset();
                guard = rtti::createInstanceInplace<ParentMutabilityGuard>(m_mutabilityGuardStorage, mutableThis);
                m_mutabilityGuardRef = guard;
            }

            return guard;
        }

        mutable my::WeakPtr<ParentMutabilityGuard> m_mutabilityGuardRef;
        mutable rtti::InstanceInplaceStorage<ParentMutabilityGuard> m_mutabilityGuardStorage;
    };

}  // namespace my::ser_detail
