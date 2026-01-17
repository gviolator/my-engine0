// #my_engine_source_file

#pragma once
#include "my/async/task_base.h"
#include "my/kernel/kernel_config.h"
#include "my/rtti/rtti_object.h"

#include <vector>


namespace my {
/**
 */
struct MY_ABSTRACT_TYPE IServiceInitialization : virtual IRttiObject
{
    MY_INTERFACE(my::IServiceInitialization, IRttiObject)

    virtual async::Task<> preInitService()
    {
        return async::Task<>::makeResolved();
    }

    virtual async::Task<> initService()
    {
        return async::Task<>::makeResolved();
    }

    virtual std::vector<rtti::TypeInfo> getServiceDependencies() const
    {
        return {};
    }
};

/**
 */
struct MY_ABSTRACT_TYPE IServiceShutdown : virtual IRttiObject
{
    MY_INTERFACE(my::IServiceShutdown, IRttiObject)

    virtual async::Task<> shutdownService() = 0;
};
}  // namespace my
