// #my_engine_source_file
#pragma once
#include "my/dispatch/dispatch_args.h"
#include "my/serialization/runtime_value.h"


namespace my
{
    struct MY_ABSTRACT_TYPE ClosureValue : RuntimeValue
    {
        MY_INTERFACE(my::ClosureValue, RuntimeValue)

        virtual Result<Ptr<>> invoke(DispatchArguments args) = 0;

    private:
        inline bool isMutable() const final
        {
            return true;
        }
    };
}  // namespace my
