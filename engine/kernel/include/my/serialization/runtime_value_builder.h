// #my_engine_source_file

#pragma once

// clang-format off

#include "my/diag/assert.h"

#include "my/serialization/native_runtime_value/native_value_forwards.h"
#include "my/serialization/native_runtime_value/native_integer_value.h"
#include "my/serialization/native_runtime_value/native_boolean_value.h"
#include "my/serialization/native_runtime_value/native_float_value.h"
#include "my/serialization/native_runtime_value/native_string_value.h"
#include "my/serialization/native_runtime_value/native_optional_value.h"
#include "my/serialization/native_runtime_value/native_collection.h"
#include "my/serialization/native_runtime_value/native_tuple.h"
#include "my/serialization/native_runtime_value/native_dictionary.h"
#include "my/serialization/native_runtime_value/native_object.h"
#include "my/memory/runtime_stack.h"

// clang format on

namespace my
{
    /**
     *
     */
    inline Ptr<RuntimeValueRef> makeValueRef(const RuntimeValuePtr& value, IAllocator* allocator)
    {
        return RuntimeValueRef::create(std::cref(value), allocator);
    }

    inline Ptr<RuntimeValueRef> makeValueRef(RuntimeValuePtr& value, IAllocator* allocator)
    {
        return RuntimeValueRef::create(value, allocator);
    }


    template <typename T>
    Result<> runtimeValueApply(T& target, const RuntimeValuePtr& rtValue)
    {
        static_assert(!std::is_const_v<T>, "Const type is passed. Use remove_const_t on call site");
        rtstack_scope;
        return RuntimeValue::assign(makeValueRef(target, getRtStackAllocatorPtr()), rtValue);
    }

    template <typename T>
    [[nodiscard]]
    Result<T> runtimeValueCast(const RuntimeValuePtr& rtValue)
    {
        static_assert(std::is_default_constructible_v<T>, "Default constructor required or use my::RuntimeValueApply");
        static_assert(!std::is_reference_v<T>, "Reference type is passed");

        Result<std::remove_const_t<T>> value{};  // << default constructor
        CheckResult(runtimeValueApply(*value, rtValue));

        return value;
    }

    template<typename T>
    requires(std::is_arithmetic_v<T>)
    [[nodiscard]]
    Result<T> runtimeValueCast(const RuntimeValuePtr& rtValue)
    {
        if (const auto* const floatValue = rtValue->as<const FloatValue*>())
        {
            return floatValue->get<T>();
        }
        else if (const auto* const intValue = rtValue->as<const IntegerValue*>())
        {
            return intValue->isSigned() ? static_cast<T>(intValue->getInt64()) : static_cast<T>(intValue->getUint64());
        }
        else if (const auto* const boolValue = rtValue->as<const BooleanValue*>())
        {
            return static_cast<T>(boolValue->getBool() ? 1u : 0u);
        }
        else if (auto* const optValue = rtValue->as<OptionalValue*>())
        {
            return !optValue->hasValue() ? static_cast<T>(0u) : runtimeValueCast<T>(optValue->getValue());
        }

        return MakeError("Can not convert to arithmetic type");
    }
}  // namespace my
