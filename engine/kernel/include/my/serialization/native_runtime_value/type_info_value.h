// #my_engine_source_file
#pragma once

#include "my/kernel/kernel_config.h"
#include "my/rtti/type_info.h"
#include "my/serialization/runtime_value.h"

namespace my::rtti
{
    struct MY_ABSTRACT_TYPE RuntimeTypeInfoValue : virtual PrimitiveValue
    {
        MY_INTERFACE(my::rtti::RuntimeTypeInfoValue, PrimitiveValue)

        virtual const TypeInfo& getTypeInfo() const = 0;

        virtual void setTypeInfo(const TypeInfo& type) = 0; 
    };


    MY_KERNEL_EXPORT RuntimeValuePtr makeValueRef(TypeInfo& value, IAllocator* allocator = nullptr);

    MY_KERNEL_EXPORT RuntimeValuePtr makeValueRef(const TypeInfo& value, IAllocator* allocator = nullptr);

    MY_KERNEL_EXPORT RuntimeValuePtr makeValueCopy(TypeInfo value, IAllocator* allocator = nullptr);
}  // namespace my::rtti
