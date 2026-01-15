// #my_engine_source_file

#pragma once

#include <memory>
#include <string_view>

#include "my/kernel/kernel_config.h"
// #include "my/rtti/rtti_object.h"
#include "my/module/module.h"
#include "my/utils/result.h"

namespace my
{
    struct IModule;

    /**
     */
    struct MY_ABSTRACT_TYPE ModuleManager
    {
        enum class ModulesPhase
        {
            Init,
            PostInit,
            Shutdown
        };

        virtual ~ModuleManager() = default;

        virtual Result<> doModulesPhase(ModulesPhase phase) = 0;

        virtual void registerModule(std::string_view moduleName, ModulePtr module) = 0;

        // virtual bool isModuleLoaded(std::string_view moduleName) = 0;

        virtual IModule* findModule(std::string_view moduleName) = 0;

        // template <typename T>
        // std::shared_ptr<T> getModule(my::hash_string moduleName)
        //{
        //     return getModule(moduleName);
        // }

        // virtual std::shared_ptr<IModule> getModuleInitialized(my::hash_string moduleName) = 0;

        // template <typename T>
        // std::shared_ptr<T> getModuleInitialized(my::hash_string moduleName)
        //{
        //     return getModuleInitialized(moduleName);
        // }

#if !MY_STATIC_RUNTIME
        // For runtime module loading
        virtual Result<> loadModule(const std::string& name, const std::string& dllPath) = 0;
#endif
    };

    using ModuleManagerPtr = std::unique_ptr<ModuleManager>;

    MY_KERNEL_EXPORT ModuleManagerPtr createModuleManager();

    MY_KERNEL_EXPORT ModuleManager& getModuleManager();

    MY_KERNEL_EXPORT bool hasModuleManager();

    MY_KERNEL_EXPORT Result<> loadModulesList(std::string_view moduleList);
}  // namespace my
