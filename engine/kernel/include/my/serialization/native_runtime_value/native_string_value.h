// #my_engine_source_file

#pragma once
#include <concepts>
#include <type_traits>

#include "my/diag/logging.h"
#include "my/rtti/rtti_impl.h"
#include "my/serialization/native_runtime_value/native_value_base.h"
#include "my/serialization/native_runtime_value/native_value_forwards.h"

namespace my::ser_detail
{

    template <typename T>
    class NativeBasicStringValue : public ser_detail::NativePrimitiveRuntimeValueBase<StringValue>
    {
        using Base = ser_detail::NativePrimitiveRuntimeValueBase<StringValue>;

        MY_REFCOUNTED_CLASS(NativeBasicStringValue<T>, Base)
    public:
        using UnderlyingString = std::decay_t<T>;

        static_assert(sizeof(typename UnderlyingString::value_type) == sizeof(char));

        static inline constexpr bool IsMutable = !std::is_const_v<std::remove_reference_t<T>>;
        static inline constexpr bool IsReference = std::is_lvalue_reference_v<T>;

        // to preserve reference and const 'T' MUST be using.
        NativeBasicStringValue(T str)
        requires(IsReference)
            :
            m_string(str)
        {
        }

        NativeBasicStringValue(const UnderlyingString& str)
        requires(!IsReference)
            :
            m_string(str)
        {
        }

        NativeBasicStringValue(UnderlyingString&& str)
        requires(!IsReference)
            :
            m_string(std::move(str))
        {
        }

        NativeBasicStringValue(const typename UnderlyingString::value_type* data, size_t size)
        requires(!IsReference)
            :
            m_string{data, size}
        {
        }

        bool isMutable() const override
        {
            return IsMutable;
        }

        Result<> setString(std::string_view str) override
        {
            if constexpr (IsMutable)
            {
                value_changes_scope;
                m_string.assign(reinterpret_cast<const UnderlyingString::value_type*>(str.data()), str.length());

                return kResultSuccess;
            }
            else
            {
                MY_FAILURE("Attempt to change non mutable string value");
                return MakeError("Attempt to change non mutable string value");
            }
        }

        std::string getString() const override
        {
            return std::string{reinterpret_cast<const char*>(m_string.data()), m_string.size()};
        }

    private:
        T m_string;
    };

    /**
     */
    template <typename T>
    class NativeStringParsableValue : public ser_detail::NativePrimitiveRuntimeValueBase<StringValue>
    {
        using Base = ser_detail::NativePrimitiveRuntimeValueBase<StringValue>;

        MY_REFCOUNTED_CLASS(NativeStringParsableValue<T>, Base)
    public:
        using ValueType = std::decay_t<T>;

        static_assert(StringParsable<ValueType>);

        static inline constexpr bool IsMutable = !std::is_const_v<std::remove_reference_t<T>>;
        static inline constexpr bool IsReference = std::is_lvalue_reference_v<T>;

        NativeStringParsableValue(T value)
        requires(IsReference)
            :
            m_value(value)
        {
        }

        NativeStringParsableValue(const ValueType& value)
        requires(!IsReference)
            :
            m_value(value)
        {
        }

        NativeStringParsableValue(ValueType&& value)
        requires(!IsReference)
            :
            m_value(std::move(value))
        {
        }

        bool isMutable() const override
        {
            return IsMutable;
        }

        Result<> setString(std::string_view str) override
        {
            if constexpr (IsMutable)
            {
                const auto result = parse(str, m_value);
                CheckResult(result);

                this->notifyChanged();
                return kResultSuccess;
            }
            else
            {
                MY_FAILURE("Attempt to change non mutable string value");
                return MakeError("Attempt to change non mutable string value");
            }

            return MakeError("Invalid code path");
        }

        std::string getString() const override
        {
            return toString(m_value);
        }

    private:
        T m_value;
    };

}  // namespace my::ser_detail

namespace my
{
    template <typename... Traits>
    Ptr<StringValue> makeValueRef(std::basic_string<char, Traits...>& str, IAllocator* allocator)
    {
        using StringType = ser_detail::NativeBasicStringValue<std::basic_string<char, Traits...>&>;

        return rtti::createInstanceWithAllocator<StringType>(std::move(allocator), str);
    }

    template <typename... Traits>
    Ptr<StringValue> makeValueRef(const std::basic_string<char, Traits...>& str, IAllocator* allocator)
    {
        using StringType = ser_detail::NativeBasicStringValue<const std::basic_string<char, Traits...>&>;

        return rtti::createInstanceWithAllocator<StringType>(std::move(allocator), str);
    }

    inline Ptr<StringValue> makeValueCopy(std::string_view str, IAllocator* allocator)
    {
        using StringType = ser_detail::NativeBasicStringValue<std::basic_string<char>>;

        return rtti::createInstanceWithAllocator<StringType>(std::move(allocator), str.data(), str.size());
    }

    // template <typename C, typename... Traits>
    // requires(sizeof(C) == sizeof(char))
    // Ptr<StringValue> makeValueRef(std::basic_string<C, Traits...>& str, IAllocator* allocator)
    // {
    //     using StringType = ser_detail::NativeBasicStringValue<std::basic_string<C, Traits...>&>;

    //     return rtti::createInstanceWithAllocator<StringType>(std::move(allocator), str);
    // }

    // template <typename C, typename... Traits>
    // requires(sizeof(C) == sizeof(char))
    // Ptr<StringValue> makeValueRef(const std::basic_string<C, Traits...>& str, IAllocator* allocator)
    // {
    //     using StringType = ser_detail::NativeBasicStringValue<const std::basic_string<C, Traits...>&>;

    //     return rtti::createInstanceWithAllocator<StringType>(std::move(allocator), str);
    // }

    // inline Ptr<StringValue> makeValueCopy(std::string_view str, IAllocator* allocator)
    // {
    //     using StringType = ser_detail::NativeBasicStringValue<std::basic_string<char>>;

    //     return rtti::createInstanceWithAllocator<StringType>(std::move(allocator), str.data(), str.size());
    // }

    // inline Ptr<StringValue> makeValueCopy(std::u8string_view str, IAllocator* allocator)
    // {
    //     using StringType = ser_detail::NativeBasicStringValue<std::basic_string<char>>;

    //     return rtti::createInstanceWithAllocator<StringType>(std::move(allocator), reinterpret_cast<const char*>(str.data()), str.size());
    // }

    template <AutoStringRepresentable T>
    Ptr<StringValue> makeValueRef(T& value, IAllocator* allocator)
    {
        using Type = ser_detail::NativeStringParsableValue<T&>;

        return rtti::createInstanceWithAllocator<Type>(std::move(allocator), value);
    }

    template <AutoStringRepresentable T>
    Ptr<StringValue> makeValueRef(const T& value, IAllocator* allocator)
    {
        using Type = ser_detail::NativeStringParsableValue<const T&>;

        return rtti::createInstanceWithAllocator<Type>(std::move(allocator), value);
    }

    template <AutoStringRepresentable T>
    Ptr<StringValue> makeValueCopy(const T& value, IAllocator* allocator)
    {
        using Type = ser_detail::NativeStringParsableValue<T>;

        return rtti::createInstanceWithAllocator<Type>(std::move(allocator), value);
    }

    template <AutoStringRepresentable T>
    Ptr<StringValue> makeValueCopy(T&& value, IAllocator* allocator)
    {
        using Type = ser_detail::NativeStringParsableValue<T>;

        return rtti::createInstanceWithAllocator<Type>(std::move(allocator), std::move(value));
    }

}  // namespace my