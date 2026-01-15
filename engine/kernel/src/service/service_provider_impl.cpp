// #my_engine_source_file

#include "service_provider_impl.h"

#include "my/runtime/async_disposable.h"
#include "my/runtime/disposable.h"

namespace my
{
    namespace
    {
        struct ServiceEntry
        {
            IServiceInitialization* service;
            bool collectingDependencies = true;
            std::set<rtti::TypeInfo> dependencies;

            ServiceEntry(IServiceInitialization* serviceIn) :
                service(serviceIn)
            {
            }

        private:
            void appendDependencies(const std::vector<rtti::TypeInfo>& typeInfoCollection)
            {
                for (const rtti::TypeInfo& type : typeInfoCollection)
                {
                    dependencies.emplace(type);
                }
            }

            void appendDependencies(const std::set<rtti::TypeInfo>& typeInfoCollection)
            {
                for (const rtti::TypeInfo& type : typeInfoCollection)
                {
                    dependencies.emplace(type);
                }
            }

            bool isDependsOn(const ServiceEntry& other) const
            {
                if (dependencies.empty() || other.service == service)
                {
                    return false;
                }

                return std::any_of(dependencies.begin(), dependencies.end(), [service = other.service](const rtti::TypeInfo& t)
                {
                    return service->is(t);
                });
            }

            friend class OrderedServiceListBuilder;
        };

        class OrderedServiceListBuilder
        {
        public:
            OrderedServiceListBuilder(std::vector<IServiceInitialization*> allServices) :
                m_allServices(std::move(allServices))
            {
            }

            std::tuple<std::list<ServiceEntry>, std::list<ServiceEntry>> takeOutServiceList()
            {
                for (IServiceInitialization* const service : m_allServices)
                {
                    [[maybe_unused]] auto& entry = getServiceEntry(service);
                }

                MY_FATAL(m_services.size() == m_allServices.size());

                m_services.sort([](const ServiceEntry& left, const ServiceEntry& right)
                {
                    return right.isDependsOn(left);
                });

                // ensure that services without dependencies have forced previous services with dependencies
                std::list<ServiceEntry> servicesWithNoDependencies;
                for (auto iter = m_services.begin(); iter != m_services.end();)
                {
                    if (iter->dependencies.empty())
                    {
                        auto temp = iter;
                        ++iter;
                        servicesWithNoDependencies.splice(servicesWithNoDependencies.end(), m_services, temp);
                    }
                    else
                    {
                        ++iter;
                    }
                }

                MY_FATAL(m_services.size() + servicesWithNoDependencies.size() == m_allServices.size());

                return {std::move(servicesWithNoDependencies), std::move(m_services)};
            }

        private:
            static bool isDependencyFor(const IServiceInitialization& service, const std::vector<rtti::TypeInfo>& types)
            {
                return std::any_of(types.begin(), types.end(), [&service](const rtti::TypeInfo& t)
                {
                    return service.is(t);
                });
            }

            ServiceEntry& getServiceEntry(IServiceInitialization* service)
            {
                auto iter = std::find_if(m_services.begin(), m_services.end(), [service](const ServiceEntry& entry)
                {
                    return entry.service == service;
                });

                if (iter == m_services.end())
                {
                    m_services.emplace_back(service);
                    auto& entry = m_services.back();
                    entry.collectingDependencies = true;

                    if (auto directDependencies = service->getServiceDependencies(); !directDependencies.empty())
                    {
                        entry.appendDependencies(directDependencies);

                        for (IServiceInitialization* const otherService : m_allServices)
                        {
                            if (otherService == service)
                            {
                                continue;
                            }

                            if (isDependencyFor(*otherService, directDependencies))
                            {
                                ServiceEntry& otherServiceEntry = getServiceEntry(otherService);
                                entry.appendDependencies(otherServiceEntry.dependencies);
                            }
                        }
                    }

                    entry.collectingDependencies = false;
                    return entry;
                }

                MY_FATAL(!iter->collectingDependencies, "Service cyclic dependency");
                return *iter;
            }

            const std::vector<IServiceInitialization*> m_allServices;

            std::list<ServiceEntry> m_services;
        };

        /**
            reorder services to takes into account the dependencies between them:
            dependencies come first, followed by other services that use them.

            returns two sequences : 1) independent services (can initialize in parallel) 2) ordered dependent services (MUST initialize sequentially)
         */
        auto makeInitOrderedServiceList(std::vector<IServiceInitialization*> allServices)
        {
            OrderedServiceListBuilder serviceListBuilder{std::move(allServices)};
            return serviceListBuilder.takeOutServiceList();
        }

        /**
            reorder services to shutdown to takes into account the dependencies between them:
            returns two sequences : 1) independent services (MUST shutdown in parallel) 2) ordered dependent services (MUST shutdown sequentially)
         */
        std::tuple<std::list<IServiceShutdown*>, std::list<IServiceShutdown*>> makeShutdownOrderedServiceList(std::vector<IServiceShutdown*> unorderedShutdownSequence)
        {
            std::vector<IServiceInitialization*> initializationList;
            initializationList.reserve(unorderedShutdownSequence.size());

            std::list<IServiceShutdown*> orderedIndependentServices;
            std::list<IServiceShutdown*> orderedDependentServices;

            for (IServiceShutdown* const serviceShutdown : unorderedShutdownSequence)
            {
                MY_FATAL(serviceShutdown);
                if (auto* const serviceInitialization = serviceShutdown->as<IServiceInitialization*>(); serviceInitialization)
                {
                    initializationList.push_back(serviceInitialization);
                }
                else
                {
                    // service without initialization: just adding it into sequence, but it will be executed after interdependent services.
                    orderedIndependentServices.push_back(serviceShutdown);
                }
            }

            if (!initializationList.empty())
            {
                auto [independentServices, dependentServices] = makeInitOrderedServiceList(initializationList);

                for (auto iter = independentServices.begin(); iter != independentServices.end(); ++iter)
                {
                    IServiceShutdown* const serviceShutdown = iter->service->as<IServiceShutdown*>();
                    MY_FATAL(serviceShutdown);
                    orderedIndependentServices.push_front(serviceShutdown);
                }

                for (auto iter = dependentServices.begin(); iter != dependentServices.end(); ++iter)
                {
                    IServiceShutdown* const serviceShutdown = iter->service->as<IServiceShutdown*>();
                    MY_FATAL(serviceShutdown);
                    // will shutdown services in reverse order of its initialization
                    orderedDependentServices.push_front(serviceShutdown);
                }
            }

            MY_DEBUG_ASSERT(orderedIndependentServices.size() + orderedDependentServices.size() == unorderedShutdownSequence.size());

            return {std::move(orderedIndependentServices), std::move(orderedDependentServices)};
        }

    }  // namespace

    ServiceProviderImpl::ServiceProviderImpl()
    {
    }

    ServiceProviderImpl::~ServiceProviderImpl()
    {
        if (!m_isDisposed)
        {
        }
    }

    void* ServiceProviderImpl::findInternal(const rtti::TypeInfo& type)
    {
        ServiceAccessor* accessor = nullptr;

        {
            const std::shared_lock lock(m_mutex);

            auto iter = m_instances.find(type);
            if (iter != m_instances.end())
            {
                return iter->second;
            }

            auto accessorIter = std::find_if(m_accessors.begin(), m_accessors.end(), [&type](const ServiceAccessorPtr& accessor)
            {
                return accessor->hasApi(type);
            });
            accessor = accessorIter != m_accessors.end() ? accessorIter->get() : nullptr;
        }

        // getApi can also access to the service provider (through lazy service creation and service impl constructor's invocation)
        void* const api = accessor ? accessor->getApi(type) : nullptr;
        if (api)
        {
            const std::lock_guard lock(m_mutex);
            m_instances.emplace(type, ServiceInstanceEntry{api, accessor});
        }

        return api;
    }

    void ServiceProviderImpl::findAllInternal(const rtti::TypeInfo& type, void (*callback)(void* instancePtr, void*), void* callbackData, ServiceAccessor::GetApiMode getApiMode)
    {
        MY_DEBUG_ASSERT(callback);
        if (!callback)
        {
            return;
        }

        // todo: use stack allocator
        std::vector<ServiceAccessor*> accessors;
        {
            const std::shared_lock lock(m_mutex);
            for (const ServiceAccessorPtr& accessor : m_accessors)
            {
                if (accessor->hasApi(type))
                {
                    accessors.emplace_back(accessor.get());
                }
            }
        }

        for (auto& accessor : accessors)
        {
            // be aware: this is normal if even accessor::hasApi(type) return true,
            // but accessor::getApi(type) return nullptr (this can be when getApiMode == GetApiMode::DoNotCreate)
            void* const api = accessor->getApi(type, getApiMode);
            if (api)
            {
                callback(api, callbackData);
            }
        }
    }

    void ServiceProviderImpl::addServiceAccessorInternal(ServiceAccessorPtr accessor, ClassDescriptorPtr classDescriptor)
    {
        MY_DEBUG_ASSERT(accessor);
        if (!accessor)
        {
            return;
        }

        const std::lock_guard lock(m_mutex);
        MY_DEBUG_ASSERT(!m_isDisposed);

        m_accessors.emplace_back(std::move(accessor));
    }

    void ServiceProviderImpl::addClass(ClassDescriptorPtr&& descriptor)
    {
        MY_DEBUG_ASSERT(descriptor);
        MY_DEBUG_ASSERT(descriptor->getInterfaceCount() > 0);

        const std::lock_guard lock(m_mutex);
        m_classDescriptors.push_back(std::move(descriptor));
    }

    ClassDescriptorPtr ServiceProviderImpl::findClass(rtti::TypeInfo type)
    {
        const std::shared_lock lock(m_mutex);
        ClassDescriptorPtr resultClass;

        for (const auto& classDescriptor : m_classDescriptors)
        {
            if (classDescriptor->findInterface(type) != nullptr)
            {
                MY_DEBUG_FATAL(!resultClass, "There is multiple classes that provides required api");
                resultClass = std::move(classDescriptor);
            }
        }

        return resultClass;
    }

    std::vector<ClassDescriptorPtr> ServiceProviderImpl::findClasses(rtti::TypeInfo t)
    {
        std::vector<ClassDescriptorPtr> classes;

        const std::shared_lock lock(m_mutex);

        for (const auto& classDescriptor : m_classDescriptors)
        {
            for (size_t i = 0, count = classDescriptor->getInterfaceCount(); i < count; ++i)
            {
                if (const rtti::TypeInfo apiType = classDescriptor->getInterface(i).getTypeInfo(); apiType && apiType == t)
                {
                    classes.push_back(classDescriptor.get());
                    break;
                }
            }
        }

        return classes;
    }

    std::vector<ClassDescriptorPtr> ServiceProviderImpl::findClasses(std::span<rtti::TypeInfo> types, bool anyType)
    {
        using MatchPredicate = bool (*)(decltype(types), const IClassDescriptor&);

        std::vector<ClassDescriptorPtr> classes;

        const MatchPredicate matchAny = [](decltype(types) types, const IClassDescriptor& classDescriptor) -> bool
        {
            return std::any_of(types.begin(), types.end(), [&classDescriptor](const rtti::TypeInfo& type)
            {
                MY_FATAL(type);
                return classDescriptor.hasInterface(type);
            });
        };

        const MatchPredicate matchAll = [](decltype(types) types, const IClassDescriptor& classDescriptor) -> bool
        {
            return std::all_of(types.begin(), types.end(), [&classDescriptor](const rtti::TypeInfo type)
            {
                MY_FATAL(type);
                return classDescriptor.hasInterface(type);
            });
        };

        const MatchPredicate predicate = anyType ? matchAny : matchAll;

        const std::shared_lock lock(m_mutex);

        for (const ClassDescriptorPtr& classDescriptor : m_classDescriptors)
        {
            if (predicate(types, *classDescriptor))
            {
                classes.push_back(classDescriptor.get());
            }
        }

        return classes;
    }

    bool ServiceProviderImpl::hasApiInternal(const rtti::TypeInfo& type)
    {
        const std::shared_lock lock(m_mutex);

        return std::any_of(m_accessors.begin(), m_accessors.end(), [&type](ServiceAccessorPtr& accessor)
        {
            return accessor->hasApi(type);
        });
    }

    template <typename T>
    T& ServiceProviderImpl::getInitializationInstance(T* instance)
    {
        MY_FATAL(instance);
        const std::shared_lock lock(m_mutex);

        if constexpr (std::is_same_v<IServiceInitialization, T>)
        {
            auto proxy = m_initializationProxy.find(instance);
            return proxy == m_initializationProxy.end() ? *instance : *proxy->second;
        }
        else
        {
            const IServiceInitialization* const serviceInit = instance->template as<const IServiceInitialization*>();
            if (auto proxyIter = m_initializationProxy.find(serviceInit); proxyIter != m_initializationProxy.end())
            {
                IServiceInitialization* const proxyServiceInit = proxyIter->second;
                T* const proxyTargetApi = proxyServiceInit->as<T*>();
                return proxyTargetApi ? *proxyTargetApi : *instance;
            }

            return *instance;
        }
    }

    async::Task<> ServiceProviderImpl::initServicesInternal(async::Task<> (*getTaskCallback)(IServiceInitialization&))
    {
        using namespace my::async;

        std::vector<IServiceInitialization*> services;
        findAllInternal(rtti::getTypeInfo<IServiceInitialization>(), [](void* servicePtr, void* serviceCollection)
        {
            reinterpret_cast<decltype(services)*>(serviceCollection)->push_back(reinterpret_cast<IServiceInitialization*>(servicePtr));
        }, &services, ServiceAccessor::GetApiMode::AllowLazyCreation);

        auto [independentServices, orderedDependentServices] = makeInitOrderedServiceList(services);

        {
            std::vector<Task<>> independentInitializationTasks;
            for (const ServiceEntry& entry : independentServices)
            {
                IServiceInitialization& serviceInstance = getInitializationInstance(entry.service);
                if (auto task = getTaskCallback(serviceInstance); task && !task.isReady())
                {
                    independentInitializationTasks.emplace_back(std::move(task));
                }
            }

            co_await whenAll(independentInitializationTasks);

#ifdef MY_DEBUG_ASSERT_ENABLED
            for (auto& task : independentInitializationTasks)
            {
                if (task.isRejected())
                {
                    MY_FAILURE(task.getError()->getDiagMessage().c_str());
                }
            }
#endif
        }

        for (const ServiceEntry& serviceEntry : orderedDependentServices)
        {
            IServiceInitialization& serviceInstance = getInitializationInstance(serviceEntry.service);
            if (Task<> task = getTaskCallback(serviceInstance); task)
            {
                co_await task;

#ifdef MY_DEBUG_ASSERT_ENABLED
                if (task.isRejected())
                {
                    MY_FAILURE(task.getError()->getDiagMessage().c_str());
                }
#endif
            }
        }
    }

    void ServiceProviderImpl::setInitializationProxy(const IServiceInitialization& source, IServiceInitialization* proxy)
    {
        const std::lock_guard lock(m_mutex);

        if (proxy)
        {
            MY_DEBUG_ASSERT(!m_initializationProxy.contains(&source), "Proxy for source already set");
            m_initializationProxy[&source] = proxy;
        }
        else
        {
            m_initializationProxy.erase(&source);
        }
    }

    async::Task<> ServiceProviderImpl::preInitServices()
    {
        return initServicesInternal([](IServiceInitialization& serviceInit)
        {
            return serviceInit.preInitService();
        });
    }

    async::Task<> ServiceProviderImpl::initServices()
    {
        return initServicesInternal([](IServiceInitialization& serviceInit)
        {
            return serviceInit.initService();
        });
    }

    async::Task<> ServiceProviderImpl::shutdownServices()
    {
        using namespace my::async;

        constexpr auto NoLazyCreation = ServiceAccessor::GetApiMode::DoNotCreate;

        {
            const std::lock_guard lock(m_mutex);
            m_isDisposed = true;
        }

        // Seems there is no need from this point to keep m_mutex locked any more:
        // m_accessors collection must not changed because shutdown

        // 1.1.creating ordered shutdown sequence: reverse order of initialization
        // 1.2 shutdown sequence consists of two sub-sequences:
        //      1) services with dependencies: must shutdown sequentially in specified order
        //      2) services without dependencies: must shutdown without blocking each other (i.e. run serviceShutdown without waiting previous)
        // 2. invoke service disposeAsync -> dispose, wait all dispose task

        {
            std::vector<IServiceShutdown*> unorderedShutdownSequence;

            for (const ServiceAccessorPtr& accessor : m_accessors)
            {
                if (void* const serviceShutdown = accessor->getApi(rtti::getTypeInfo<IServiceShutdown>(), NoLazyCreation))
                {
                    unorderedShutdownSequence.push_back(reinterpret_cast<IServiceShutdown*>(serviceShutdown));
                }
            }

            auto [independentServices, orderedDependentServices] = makeShutdownOrderedServiceList(unorderedShutdownSequence);

            for (IServiceShutdown* const serviceShutdown : orderedDependentServices)
            {
                if (auto task = getInitializationInstance(serviceShutdown).shutdownService(); task)
                {
                    co_await task;
                }
            }

            std::vector<Task<>> shutdownIndependentTasks;
            for (IServiceShutdown* const serviceShutdown : independentServices)
            {
                if (auto task = getInitializationInstance(serviceShutdown).shutdownService(); task && !task.isReady())
                {
                    shutdownIndependentTasks.emplace_back(std::move(task));
                }
            }

            co_await whenAll(shutdownIndependentTasks);
        }

        {
            std::vector<Task<>> disposeTasks;

            {
                for (const ServiceAccessorPtr& accessor : m_accessors)
                {
                    // invoke disposeAsync first.
                    // same class can provide both IAsyncDisposable and IDisposable api,
                    // so first async version must be called and in this case dispose can do nothing (because if async variant called).
                    if (void* const asyncDisposable = accessor->getApi(rtti::getTypeInfo<IAsyncDisposable>(), NoLazyCreation))
                    {
                        Task<> task = reinterpret_cast<IAsyncDisposable*>(asyncDisposable)->disposeAsync();
                        if (task && !task.isReady())
                        {
                            disposeTasks.push_back(std::move(task));
                        }
                    }

                    if (void* const disposable = accessor->getApi(rtti::getTypeInfo<IDisposable>(), NoLazyCreation))
                    {
                        reinterpret_cast<IDisposable*>(disposable)->dispose();
                    }
                }
            }

            co_await whenAll(disposeTasks);
        }
    }

    namespace
    {
        ServiceProviderPtr& getServiceProviderInstanceRef()
        {
            static ServiceProviderPtr s_serviceProvider;
            return (s_serviceProvider);
        }
    }  // namespace

    ServiceProviderPtr createServiceProvider()
    {
        return std::make_unique<ServiceProviderImpl>();
    }

    void setDefaultServiceProvider(ServiceProviderPtr&& provider)
    {
        MY_FATAL(!provider || !getServiceProviderInstanceRef(), "Service provider already set");
        getServiceProviderInstanceRef() = std::move(provider);
    }

    bool hasServiceProvider()
    {
        return static_cast<bool>(getServiceProviderInstanceRef());
    }

    ServiceProvider& getServiceProvider()
    {
        auto& instance = getServiceProviderInstanceRef();
        MY_FATAL(instance);
        return *instance;
    }

}  // namespace my
