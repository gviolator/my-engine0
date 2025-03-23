// #my_engine_source_header


#pragma once

#include <chrono>
#include <optional>

#include "my/async/executor.h"
#include "my/async/task.h"
#include "my/rtti/ptr.h"
#include "my/runtime/async_disposable.h"
#include "my/runtime/disposable.h"
#include "my/kernel/kernel_config.h"

namespace my
{
    struct MY_ABSTRACT_TYPE WorkQueue : async::Executor
    {
        MY_INTERFACE(my::WorkQueue, async::Executor)

        using Ptr = my::Ptr<WorkQueue>;

        MY_KERNEL_EXPORT static WorkQueue::Ptr create();
        

        virtual async::Task<> waitForWork() = 0;

        virtual void poll(std::optional<std::chrono::milliseconds> time = std::chrono::milliseconds{0}) = 0;

        virtual void notify() = 0;

        virtual void setName(std::string name) = 0;

        virtual std::string getName() const = 0;
    };

}  // namespace my
