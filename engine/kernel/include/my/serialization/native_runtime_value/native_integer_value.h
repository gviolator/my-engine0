// #my_engine_source_file

#pragma once
#include <type_traits>

#include "my/diag/check.h"
#include "my/rtti/rtti_impl.h"
#include "my/serialization/native_runtime_value/native_value_base.h"

namespace my::ser_detail
{

    /**
     */
    template <typename T>
    class NativeIntegerValue final : public ser_detail::NativePrimitiveRuntimeValueBase<RuntimeIntegerValue>
    {
        /**
            template <typename T> -> template<std::integral T>
            must not be used (because T can be reference/const reference, so std::integral<T> will fail).
        */

        using Base = ser_detail::NativePrimitiveRuntimeValueBase<RuntimeIntegerValue>;
        using IntegralValueType = std::remove_const_t<std::remove_reference_t<T>>;

        MY_REFCOUNTED_CLASS(NativeIntegerValue<T>, Base)

    public:
        static inline constexpr bool IsMutable = !std::is_const_v<std::remove_reference_t<T>>;

        NativeIntegerValue(T value) :
            m_value(value)
        {
        }

        bool isMutable() const override
        {
            return IsMutable;
        }

        bool isSigned() const override
        {
            return std::is_signed_v<IntegralValueType>;
        }

        size_t getBitsCount() const override
        {
            return sizeof(IntegralValueType);
        }

        void setInt64([[maybe_unused]] int64_t value) override
        {
            if constexpr(IsMutable)
            {
                value_changes_scope;
                m_value = static_cast<IntegralValueType>(value);
            }
            else
            {
                MY_DEBUG_FAILURE("Attempt to modify non mutable value");
            }
        }

        void setUint64([[maybe_unused]] uint64_t value) override
        {
            if constexpr(IsMutable)
            {
                value_changes_scope;
                m_value = static_cast<IntegralValueType>(value);
            }
            else
            {
                MY_FAILURE("Attempt to modify non mutable value");
            }
        }

        int64_t getInt64() const override
        {
            return static_cast<int64_t>(m_value);
        }

        uint64_t getUint64() const override
        {
            return static_cast<uint64_t>(m_value);
        }

    private:
        T m_value;
    };

}  // namespace my::ser_detail

namespace my
{
    template <std::integral T>
    Ptr<RuntimeIntegerValue> makeValueRef(T& value, MemAllocator* allocator)
    {
        return rtti::createInstanceWithAllocator<ser_detail::NativeIntegerValue<T&>>(allocator, value);
    }

    template <std::integral T>
    Ptr<RuntimeIntegerValue> makeValueRef(const T& value, MemAllocator* allocator)
    {
        return rtti::createInstanceWithAllocator<ser_detail::NativeIntegerValue<const T&>>(allocator, value);
    }

    template <std::integral T>
    Ptr<RuntimeIntegerValue> makeValueCopy(T value, MemAllocator* allocator)
    {
        return rtti::createInstanceWithAllocator<ser_detail::NativeIntegerValue<T>>(allocator, value);
    }
}  // namespace my
