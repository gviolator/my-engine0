// #my_engine_source_file
// nau/module/module_entry.h


#pragma once

#if !defined(MY_MODULE_NAME) && !defined(MY_KERNEL_BUILD)
    #error "module.h must be used only for module build"
#endif


#include <type_traits>

#include "EASTL/shared_ptr.h"
#include "EASTL/internal/smart_ptr.h"
#include "my/kernel/kernel_config.h"
#include "my/rtti/ptr.h"
#include "my/rtti/rtti_impl.h"
#include "my/service/service_provider.h"
#include "my/utils/functor.h"
#include <nau/string/string.h>


namespace my
{

    struct IModule
    {
        virtual string getModuleName() = 0; // could be usefull for Editor

        virtual void initialize() = 0;

        virtual void deinitialize() = 0;

        virtual void postInit() = 0;

        virtual ~IModule() {}

    };


#if defined(MY_STATIC_RUNTIME)
    #define IMPLEMENT_MODULE( ModuleClass ) \
        std::shared_ptr<my::IModule> PP_CONCATENATE(createModule_, MY_MODULE_NAME)() \
        { \
            return std::make_shared<ModuleClass>(); \
        } \

#else
    #define IMPLEMENT_MODULE( ModuleClass ) \
        extern "C" __declspec(dllexport) my::IModule* createModule() \
        { \
            return new ModuleClass(); \
        } \

#endif

    struct DefaultModuleImpl : public IModule
    {
        string getModuleName() override
        {
            return "DefaultModuleImpl";
        }

        void initialize() override
        {
        }

        void deinitialize() override
        {
        }

        void postInit() override
        {
        }
    };


}  // namespace my


namespace eastl
{
    template<>
    struct default_delete<my::IModule>
    {
        void operator()(my::IModule* p) const
           { delete p; }
    };
}


#define MY_MODULE_EXPORT_CLASS(Type) ::my::getServiceProvider().addClass<Type>()

#define MY_MODULE_EXPORT_SERVICE(Type) ::my::getServiceProvider().addService<Type>()
