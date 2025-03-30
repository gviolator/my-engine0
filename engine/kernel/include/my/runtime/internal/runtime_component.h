// #my_engine_source_file
#pragma once

#include "my/rtti/rtti_object.h"

namespace my
{

    /**
     */
    struct MY_ABSTRACT_TYPE IRuntimeComponent : virtual IRttiObject
    {
        MY_INTERFACE(my::IRuntimeComponent, IRttiObject)

        virtual bool hasWorks() = 0;
    };

}  // namespace my
