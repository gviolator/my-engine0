// #my_engine_source_file
#pragma once

#include "my/rtti/rtti_impl.h"
#include "my/service/internal/service_provider_initialization.h"
#include "my/service/service.h"
#include "my/service/service_provider.h"
#include "my/utils/scope_guard.h"

namespace my
{
    class ServiceProviderImpl final : public ServiceProvider,
                                      public core_detail::IServiceProviderInitialization
    {
        MY_RTTI_CLASS(my::ServiceProviderImpl, ServiceProvider, core_detail::IServiceProviderInitialization)
    public:
        ServiceProviderImpl();

        ~ServiceProviderImpl();

    private:
        struct ServiceInstanceEntry
        {
            void* const serviceInstance;
            ServiceAccessor* const accessor;

            ServiceInstanceEntry(void* inServiceInstance, ServiceAccessor* inAccessor) :
                serviceInstance(inServiceInstance),
                accessor(inAccessor)
            {
            }

            operator void*() const
            {
                return serviceInstance;
            }
        };

        void* findInternal(const rtti::TypeInfo&) override;

        void findAllInternal(const rtti::TypeInfo&, void (*)(void* instancePtr, void*), void*, ServiceAccessor::GetApiMode) override;

        void addServiceAccessorInternal(ServiceAccessor::Ptr, ClassDescriptorPtr) override;

        void addClass(ClassDescriptorPtr&& descriptor) override;

        std::vector<ClassDescriptorPtr> findClasses(rtti::TypeInfo type) override;

        std::vector<ClassDescriptorPtr> findClasses(std::span<rtti::TypeInfo>, bool anyType) override;

        bool hasApiInternal(const rtti::TypeInfo&) override;

        void setInitializationProxy(const IServiceInitialization& source, IServiceInitialization* proxy) override;

        async::Task<> preInitServices() override;

        async::Task<> initServices() override;

        async::Task<> shutdownServices() override;

        async::Task<> initServicesInternal(async::Task<> (*)(IServiceInitialization&));

        template <typename T>
        T& getInitializationInstance(T* instance);

        std::list<ServiceAccessor::Ptr> m_accessors;
        std::unordered_map<rtti::TypeInfo, ServiceInstanceEntry> m_instances;
        std::vector<ClassDescriptorPtr> m_classDescriptors;
        std::unordered_map<const IServiceInitialization*, IServiceInitialization*> m_initializationProxy;
        std::shared_mutex m_mutex;
        bool m_isDisposed = false;
    };
}  // namespace my
