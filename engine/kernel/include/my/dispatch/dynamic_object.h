// #my_engine_source_file

#pragma once

#include "my/dispatch/class_descriptor.h"
#include "my/rtti/ptr.h"
#include "my/serialization/runtime_value.h"

namespace my
{
    /**
     */
    struct MY_ABSTRACT_TYPE DynamicObject : virtual IRttiObject
    {
        MY_INTERFACE(my::DynamicObject, IRttiObject)

        /**
        */
        virtual ClassDescriptorPtr getClassDescriptor() = 0;

        /**
        */
        virtual RuntimeValuePtr getProperties() = 0;
    };
}  // namespace my
