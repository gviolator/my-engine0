#pragma once
// #my_engine_source_file
#pragma once
#include <memory>

#include "my/rtti/rtti_object.h"
#include "my/utils/result.h"

namespace my
{
    struct MY_ABSTRACT_TYPE ApplicationInitDelegate : IRttiObject
    {
        virtual ~ApplicationInitDelegate() = default;

        virtual Result<> configureApplication() = 0;

        virtual Result<> registerApplicationServices() = 0;
    };

    using ApplicationInitDelegatePtr = std::unique_ptr<ApplicationInitDelegate>;

}  // namespace my
