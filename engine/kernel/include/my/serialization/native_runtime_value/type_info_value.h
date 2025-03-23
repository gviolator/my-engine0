// #my_engine_source_header
#pragma once

#include "my/kernel/kernel_config.h"
#include "my/rtti/type_info.h"
#include "my/serialization/runtime_value.h"

namespace my::rtti
{
    struct MY_ABSTRACT_TYPE RuntimeTypeInfoValue : virtual RuntimePrimitiveValue
    {
        MY_INTERFACE(my::rtti::RuntimeTypeInfoValue, RuntimePrimitiveValue)

        virtual const TypeInfo& getTypeInfo() const = 0;

        virtual void setTypeInfo(const TypeInfo& type) = 0; 
    };


    MY_KERNEL_EXPORT RuntimeValue::Ptr makeValueRef(TypeInfo& value, IMemAllocator* allocator = nullptr);

    MY_KERNEL_EXPORT RuntimeValue::Ptr makeValueRef(const TypeInfo& value, IMemAllocator* allocator = nullptr);

    MY_KERNEL_EXPORT RuntimeValue::Ptr makeValueCopy(TypeInfo value, IMemAllocator* allocator = nullptr);
}  // namespace my::rtti
