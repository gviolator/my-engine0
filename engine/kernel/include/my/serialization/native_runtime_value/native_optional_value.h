// #my_engine_source_file

#pragma once

#include <type_traits>

#include "my/diag/assert.h"
#include "my/rtti/rtti_impl.h"
#include "my/serialization/native_runtime_value/native_value_base.h"
#include "my/serialization/native_runtime_value/native_value_forwards.h"

namespace my::ser_detail
{
    /**
     */
    template <typename T>
    class StdOptionalValue final : public ser_detail::NativeRuntimeValueBase<OptionalValue>
    {
        using Base = ser_detail::NativeRuntimeValueBase<OptionalValue>;
        using OptionalType = std::decay_t<T>;

        MY_REFCOUNTED_CLASS(StdOptionalValue<T>, Base)

    public:
        static_assert(LikeStdOptional<OptionalType>);
        static inline constexpr bool IsMutable = !std::is_const_v<std::remove_reference_t<T>>;
        static inline constexpr bool IsReference = std::is_lvalue_reference_v<T>;

        StdOptionalValue(T optionalValue)
        requires(IsReference)
            :
            m_optional(optionalValue)
        {
        }

        StdOptionalValue(const OptionalType& optionalValue)
        requires(!IsReference)
            :
            m_optional(optionalValue)
        {
        }

        StdOptionalValue(OptionalType&& optionalValue)
        requires(!IsReference)
            :
            m_optional(std::move(optionalValue))
        {
        }

        bool isMutable() const override
        {
            return IsMutable;
        }

        bool hasValue() const override
        {
            return m_optional.has_value();
        }

        RuntimeValuePtr getValue() override
        {
            if(!hasValue())
            {
                return nullptr;
            }

            return this->makeChildValue(makeValueRef(m_optional.value()));
        }

        Result<> setValue(RuntimeValuePtr value) override
        {
            if constexpr(IsMutable)
            {
                value_changes_scope;
                if(!value)
                {
                    m_optional.reset();
                    return ResultSuccess;
                }

                if (!m_optional.has_value())
                {
                    static_assert(std::is_default_constructible_v<typename OptionalType::value_type>);
                    m_optional.emplace();
                }

                decltype(auto) myValue = m_optional.value();
                return RuntimeValue::assign(makeValueRef(myValue), std::move(value));
            }
            else
            {
                MY_DEBUG_FAILURE("Attempt to modify non mutable value");
                return MakeError("Attempt to modify non mutable optional value");
            }
        }

    private:
        T m_optional;
    };

}  // namespace my::ser_detail

namespace my
{
    template <LikeStdOptional T>
    Ptr<OptionalValue> makeValueRef(T& opt, IAllocator* allocator)
    {
        using Optional = ser_detail::StdOptionalValue<T&>;

        return rtti::createInstanceWithAllocator<Optional>(allocator, opt);
    }

    template <LikeStdOptional T>
    Ptr<OptionalValue> makeValueRef(const T& opt, IAllocator* allocator)
    {
        using Optional = ser_detail::StdOptionalValue<const T&>;

        return rtti::createInstanceWithAllocator<Optional>(allocator, opt);
    }

    template <LikeStdOptional T>
    Ptr<OptionalValue> makeValueCopy(const T& opt, IAllocator* allocator)
    {
        using Optional = ser_detail::StdOptionalValue<T>;

        return rtti::createInstanceWithAllocator<Optional>(allocator, opt);
    }

    template <LikeStdOptional T>
    Ptr<OptionalValue> makeValueCopy(T&& opt, IAllocator* allocator)
    {
        using Optional = ser_detail::StdOptionalValue<T>;

        return rtti::createInstanceWithAllocator<Optional>(std::move(allocator), std::move(opt));
    }
}  // namespace my
