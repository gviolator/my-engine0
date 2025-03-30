// #my_engine_source_file


#pragma once

#include "my/async/task.h"
#include "my/rtti/type_info.h"

namespace my
{
    /**
     */
    struct MY_ABSTRACT_TYPE IAsyncDisposable
    {
        MY_TYPEID(my::IAsyncDisposable)

        virtual async::Task<> disposeAsync() = 0;
    };

}  // namespace my
