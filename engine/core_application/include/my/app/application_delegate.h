#pragma once

#include "my/rtti/ptr.h"
#include "my/rtti/rtti_object.h"
#include "my/utils/result.h"

namespace my {

/**
*/
struct MY_ABSTRACT_TYPE ApplicationInitDelegate : IRefCounted
{
    MY_INTERFACE(my::ApplicationInitDelegate, IRefCounted)

    virtual ~ApplicationInitDelegate() = default;

    virtual Result<> configureApplication() = 0;

    virtual Result<> registerApplicationServices() = 0;
};

using ApplicationInitDelegatePtr = Ptr<ApplicationInitDelegate>;

}  // namespace my
