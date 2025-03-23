// #my_engine_source_header
#pragma once

#include <memory>

#include "my/kernel/kernel_config.h"
#include "my/utils/functor.h"

namespace my
{
    struct RuntimeState
    {
        using Ptr = std::unique_ptr<RuntimeState>;

        MY_KERNEL_EXPORT
        static RuntimeState::Ptr create();

        virtual ~RuntimeState() = default;

        virtual Functor<bool()> shutdown(bool doCompleteShutdown = true) = 0;

        virtual void completeShutdown() = 0;
    };
}  // namespace my
