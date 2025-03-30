// #my_engine_source_file
// nau/runtime/disposable.h


#pragma once

#include "my/rtti/rtti_object.h"

namespace my
{
    struct MY_ABSTRACT_TYPE IDisposable : virtual IRttiObject
    {
        MY_INTERFACE(my::IDisposable, IRttiObject)

        virtual void dispose() = 0;
    };

} // namespace my