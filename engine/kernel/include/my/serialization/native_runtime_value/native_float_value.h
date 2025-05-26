// #my_engine_source_file

#pragma once
#include <type_traits>

#include "my/diag/assert.h"
#include "my/rtti/rtti_impl.h"
#include "my/serialization/native_runtime_value/native_value_base.h"

namespace my::ser_detail
{

    /**
     *
     */
    template <typename T>
    class NativeFloatValue final : public ser_detail::NativePrimitiveRuntimeValueBase<RuntimeFloatValue>
    {
        using Base = ser_detail::NativePrimitiveRuntimeValueBase<RuntimeFloatValue>;
        using FloatValueType = std::remove_const_t<std::remove_reference_t<T>>;

        MY_REFCOUNTED_CLASS(NativeFloatValue<T>, Base)

    public:
        static inline constexpr bool IsMutable = !std::is_const_v<std::remove_reference_t<T>>;

        using ValueType = std::remove_const_t<T>;

        NativeFloatValue(T value) :
            m_value(value)
        {
        }

        bool isMutable() const override
        {
            return IsMutable;
        }

        size_t getBitsCount() const override
        {
            return sizeof(FloatValueType);
        }

        void setDouble([[maybe_unused]] double value) override
        {
            if constexpr(IsMutable)
            {
                value_changes_scope;
                m_value = static_cast<FloatValueType>(value);
            }
            else
            {
                MY_FAILURE("Attempt to modify non mutable runtime value");
            }
        }

        void setSingle([[maybe_unused]] float value) override
        {
            if constexpr(IsMutable)
            {
                value_changes_scope;
                m_value = static_cast<FloatValueType>(value);
            }
            else
            {
                MY_DEBUG_FAILURE("Attempt to modify non mutable runtime value");
            }
        }

        double getDouble() const override
        {
            return static_cast<double>(m_value);
        }

        float getSingle() const override
        {
            // todo check overflow
            return static_cast<float>(m_value);
        }

    private:
        T m_value;
    };

}  // namespace my::ser_detail

namespace my
{

    template <std::floating_point T>
    Ptr<RuntimeFloatValue> makeValueRef(T& value, IMemAllocator* allocator)
    {
        return rtti::createInstanceWithAllocator<ser_detail::NativeFloatValue<T&>>(allocator, value);
    }

    template <std::floating_point T>
    Ptr<RuntimeFloatValue> makeValueRef(const T& value, IMemAllocator* allocator)
    {
        return rtti::createInstanceWithAllocator<ser_detail::NativeFloatValue<const T&>>(allocator, value);
    }

    template <std::floating_point T>
    Ptr<RuntimeFloatValue> makeValueCopy(T value, IMemAllocator* allocator)
    {
        return rtti::createInstanceWithAllocator<ser_detail::NativeFloatValue<T>>(allocator, value);
    }
}  // namespace my
