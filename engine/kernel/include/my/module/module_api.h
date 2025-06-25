// #my_engine_source_file

#pragma once

#if !defined(MY_MODULE_NAME) && !defined(MY_KERNEL_BUILD)
    #error "module_api.h must be used only for module build"
#endif

#include "my/module/module.h"
#include "my/service/service_provider.h"

#if defined(MY_STATIC_RUNTIME)
    #define MY_DECLARE_MODULE(ModuleClass)                             \
        my::ModulePtr PP_CONCATENATE(createModule_, MY_MODULE_NAME)() \
        {                                                             \
            return std::make_unique<ModuleClass>();                   \
        }

#else
    #define MY_DECLARE_MODULE(ModuleClass)                            \
        extern "C" __declspec(dllexport) my::IModule* createModule() \
        {                                                            \
            return new ModuleClass();                                \
        }

#endif

#define MY_MODULE_EXPORT_CLASS(Type) ::my::getServiceProvider().addClass<Type>()

#define MY_MODULE_EXPORT_SERVICE(Type) ::my::getServiceProvider().addService<Type>()
