// #my_engine_source_file


#pragma once

#include "my/async/task_base.h"
#include "my/rtti/rtti_object.h"

namespace my
{
    struct IServiceInitialization;
}

namespace my::core_detail
{

    /**
        @brief internal API dedicated to manage service initialization and shutting down phases
     */
    struct MY_ABSTRACT_TYPE IServiceProviderInitialization
    {
        MY_TYPEID(my::core_detail::IServiceProviderInitialization);

        virtual ~IServiceProviderInitialization() = default;

        /**
            @brief setting initialization proxy for specified service.

            In some cases, a service may require a specific initialization procedure (for example, a call must occur in a dedicated thread).
            A proxy object can be used for this. If a proxy is installed, all IServiceInitialization calls will be directed to it.
            If proxy exposes IServiceShutdown API then shutdownService() will also be called for proxy object
            (otherwise shutdownService() will be called for original object).

            IMPORTANT! To use the proxy mechanism, the source type must also expose IServiceInitialization/IServiceShutdown interfaces
            (this is required because service inter dependencies are computed for original types, not proxies, also other types knowns nothing about proxies.
            i.e. IServiceInitialization::getServiceDependencies() for proxy are ignored and never be called).
         */
        virtual void setInitializationProxy(const IServiceInitialization& source, IServiceInitialization* proxy) = 0;

        virtual async::Task<> preInitServices() = 0;

        virtual async::Task<> initServices() = 0;

        virtual async::Task<> shutdownServices() = 0;
    };
}  // namespace my::core_detail