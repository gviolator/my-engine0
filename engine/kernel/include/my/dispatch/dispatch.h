// #my_engine_source_file

#pragma once
#include <optional>
#include <string_view>
#include <vector>

#include "my/dispatch/class_descriptor.h"
#include "my/dispatch/dispatch_args.h"
#include "my/meta/class_info.h"
#include "my/rtti/ptr.h"
#include "my/rtti/rtti_object.h"
#include "my/serialization/runtime_value.h"
#include "my/utils/result.h"

namespace my
{
    /**
     */
    struct MY_ABSTRACT_TYPE IDispatch : virtual IRttiObject
    {
        MY_INTERFACE(IDispatch, IRttiObject)

        virtual Result<Ptr<>> invoke(std::string_view contract, std::string_view method, DispatchArguments args) = 0;

        virtual ClassDescriptorPtr getClassDescriptor() const = 0;
    };

}  // namespace my