// #my_engine_source_file
#pragma once

#include <memory>

#include "my/kernel/kernel_config.h"
#include "my/utils/functor.h"

namespace my
{
    struct KernelRuntime
    {
        virtual ~KernelRuntime() = default;

        /**
            @p resetKernelServices
                if true then shutdown will reset kernel services
                in other case reset will be performed within KernelRuntime's destructor.
        */
        virtual Functor<bool()> shutdown(bool resetKernelServices = true) = 0;
    };

    using KernelRuntimePtr = std::unique_ptr<KernelRuntime>;

    MY_KERNEL_EXPORT KernelRuntimePtr createKernelRuntime();

}  // namespace my
