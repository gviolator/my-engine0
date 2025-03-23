// #my_engine_source_header

#pragma once

#include <type_traits>

#include "my/diag/check.h"
#include "my/rtti/rtti_impl.h"
#include "my/serialization/native_runtime_value/native_value_base.h"
#include "my/serialization/native_runtime_value/native_value_forwards.h"

namespace my::ser_detail
{
    /**
     */
    template <typename T>
    class StdOptionalValue final : public ser_detail::NativeRuntimeValueBase<RuntimeOptionalValue>
    {
        using Base = ser_detail::NativeRuntimeValueBase<RuntimeOptionalValue>;
        using OptionalType = std::decay_t<T>;

        MY_CLASS_(StdOptionalValue<T>, Base)

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

        RuntimeValue::Ptr getValue() override
        {
            if(!hasValue())
            {
                return nullptr;
            }

            return this->makeChildValue(makeValueRef(m_optional.value()));
        }

        Result<> setValue(RuntimeValue::Ptr value) override
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
    RuntimeOptionalValue::Ptr makeValueRef(T& opt, IMemAllocator::Ptr allocator)
    {
        using Optional = ser_detail::StdOptionalValue<T&>;

        return rtti::createInstanceWithAllocator<Optional>(std::move(allocator), opt);
    }

    template <LikeStdOptional T>
    RuntimeOptionalValue::Ptr makeValueRef(const T& opt, IMemAllocator::Ptr allocator)
    {
        using Optional = ser_detail::StdOptionalValue<const T&>;

        return rtti::createInstanceWithAllocator<Optional>(std::move(allocator), opt);
    }

    template <LikeStdOptional T>
    RuntimeOptionalValue::Ptr makeValueCopy(const T& opt, IMemAllocator::Ptr allocator)
    {
        using Optional = ser_detail::StdOptionalValue<T>;

        return rtti::createInstanceWithAllocator<Optional>(std::move(allocator), opt);
    }

    template <LikeStdOptional T>
    RuntimeOptionalValue::Ptr makeValueCopy(T&& opt, IMemAllocator::Ptr allocator)
    {
        using Optional = ser_detail::StdOptionalValue<T>;

        return rtti::createInstanceWithAllocator<Optional>(std::move(allocator), std::move(opt));
    }
}  // namespace my
