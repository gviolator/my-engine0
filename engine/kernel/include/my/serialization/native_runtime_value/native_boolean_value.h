// #my_engine_source_header

#pragma once
#include <concepts>
#include <type_traits>

#include "my/diag/check.h"
#include "my/rtti/rtti_impl.h"
#include "my/serialization/native_runtime_value/native_value_base.h"

namespace my
{
    /**
     */
    template <typename T>
    class NativeBooleanValue final : public ser_detail::NativePrimitiveRuntimeValueBase<RuntimeBooleanValue>
    {
        using Base = ser_detail::NativePrimitiveRuntimeValueBase<RuntimeBooleanValue>;

        MY_CLASS_(NativeBooleanValue<T>, Base)

    public:
        static inline constexpr bool IsMutable = !std::is_const_v<std::remove_reference_t<T>>;

        NativeBooleanValue(T value) :
            m_value(value)
        {
        }

        bool isMutable() const override
        {
            return IsMutable;
        }

        void setBool(bool value) override
        {
            if constexpr(IsMutable)
            {
                value_changes_scope;
                m_value = value;
            }
            else
            {
                MY_DEBUG_FAILURE("Attempt to modify non mutable value");
            }
        }

        bool getBool() const override
        {
            return m_value;
        }

    private:
        T m_value;
    };

    /**
    */
    inline RuntimeBooleanValue::Ptr makeValueRef(bool& value, IMemAllocator::Ptr allocator)
    {
        return rtti::createInstanceWithAllocator<NativeBooleanValue<bool&>>(std::move(allocator), value);
    }

    inline RuntimeBooleanValue::Ptr makeValueRef(const bool& value, IMemAllocator::Ptr allocator)
    {
        return rtti::createInstanceWithAllocator<NativeBooleanValue<const bool&>>(std::move(allocator), value);
    }

    inline RuntimeBooleanValue::Ptr makeValueCopy(bool value, IMemAllocator::Ptr allocator)
    {
        return rtti::createInstanceWithAllocator<NativeBooleanValue<bool>>(std::move(allocator), value);
    }

}  // namespace my
