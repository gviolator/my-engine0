    // #my_engine_source_file
#include "my/module/module_manager.h"

#include "my/io/file_system.h"
#include "my/memory/runtime_stack.h"
#include "my/memory/singleton_memop.h"
#include "my/module/module.h"
#include "my/platform/windows/diag/win_error.h"
#include "my/threading/lock_guard.h"
#include "my/utils/string_conv.h"
#include "shared_library.h"


#if defined(MY_STATIC_RUNTIME)
namespace my::module_detail
{
    extern void initializeAllStaticModules(ModuleManager& manager);

}  // namespace my::module_detail
#endif

namespace my
{
    namespace
    {
        struct ModuleEntry
        {
            std::string name;
            ModulePtr module;

#if !defined(MY_STATIC_RUNTIME)
            SharedLibrary sharedLib;
#endif
        };
    }  // namespace

    /**
     */
    class ModuleManagerImpl final : public ModuleManager
    {
        MY_SINGLETON_MEMOPS(ModuleManagerImpl)
    public:
        ModuleManagerImpl()
        {
            MY_DEBUG_ASSERT(s_instance == nullptr);
            s_instance = this;
        }

        ~ModuleManagerImpl()
        {
            shutdownModules();

            MY_DEBUG_ASSERT(s_instance == this);
            s_instance = nullptr;
        }

        Result<> doModulesPhase(ModulesPhase phase) override
        {
            if (phase == ModulesPhase::Init)
            {
                return initModules();
            }
            else if (phase == ModulesPhase::PostInit)
            {
                return postInitModules();
            }
            else if (phase == ModulesPhase::Shutdown)
            {
                shutdownModules();
            }

            return ResultSuccess;
        }

        void registerModule(std::string_view inModuleName, ModulePtr module) override
        {
            MY_DEBUG_ASSERT(module);
            if (!module)
            {
                return;
            }

            const std::lock_guard lock(m_mutex);

            MY_DEBUG_ASSERT(findModuleInternal(inModuleName) == nullptr, "Module ({}) already registered", inModuleName);

            std::string moduleName {inModuleName};
            m_modules.emplace_back(ModuleEntry{.name = std::move(moduleName), .module = std::move(module)});
        }

        IModule* findModule(std::string_view moduleName) override
        {
            const std::lock_guard lock(m_mutex);
            return findModuleInternal(moduleName);
        }

#if !defined(MY_STATIC_RUNTIME)
        Result<> loadModule(const std::string& name, const std::string& dllPath) override
        {
            namespace fs = std::filesystem;

            const std::lock_guard lock(m_mutex);

            if (moduleRegistry.count(hName))
            {
                // Module already loaded
                return ResultSuccess;
            }

            eastl::wstring dllWStringPath = strings::utf8ToWString(dllPath);

            HMODULE hmodule = GetModuleHandleW(dllWStringPath.c_str());
            if (!hmodule)
            {
                hmodule = LoadLibraryW(dllWStringPath.c_str());
                if (!hmodule)
                {
                    const auto errCode = diag::getAndResetLastErrorCode();
                    NAU_LOG_ERROR("Fail to load library ({}) with error:()", dllPath, diag::getWinErrorMessageA(errCode));
                    return NauMakeError("Fail to load library ({})({}):({})", dllPath.tostring(), errCode, diag::getWinErrorMessageA(errCode));
                }
            }

            ModuleEntry entry{};
            entry.name = name;
            entry.dllPath = dllWStringPath;
            entry.dllHandle = hmodule;

            createModuleFunctionPtr createModuleFuncPtr = (createModuleFunctionPtr)GetProcAddress(entry.dllHandle, "createModule");
            if (!createModuleFuncPtr)
            {
                NAU_LOG_ERROR("Can't get proc address, module:({})", dllPath);
                return NauMakeError("Module does not export 'createModule' function");
            }

            eastl::shared_ptr<IModule> iModule = eastl::shared_ptr<IModule>(createModuleFuncPtr());
            if (!iModule)
            {
                NAU_LOG_ERROR("Invalid module object, module:({})", dllPath);
                return NauMakeError("Invalid module object");
            }

            entry.iModule = iModule;
            moduleRegistry[hName] = entry;

            if (m_needInitializeNewModules)
            {
                moduleRegistry[hName].isModuleInitialized = true;
                moduleRegistry[hName].iModule->initialize();
            }

            return ResultSuccess;
        }
#endif

    private:
        inline static ModuleManagerImpl* s_instance = nullptr;

        IModule* findModuleInternal(std::string_view moduleName) const
        {
            if (moduleName.empty())
            {
                MY_DEBUG_FAILURE("Module name can not be empty");
                return nullptr;
            }

            auto it = std::find_if(m_modules.begin(), m_modules.end(), [moduleName](const ModuleEntry& entry)
            {
                return entry.name == moduleName;
            });
            return it != m_modules.end() ? it->module.get() : nullptr;
        }

        Result<> initModules()
        {
#if MY_DEBUG_ASSERT_ENABLED
            {
                const std::lock_guard lock(m_mutex);
                MY_DEBUG_ASSERT(m_nextPhase == ModulesPhase::Init);
            }
#endif

#if defined(MY_STATIC_RUNTIME) 
            module_detail::initializeAllStaticModules(*this);
#endif
            const std::lock_guard lock(m_mutex);

            MY_DEBUG_ASSERT(m_nextPhase == ModulesPhase::Init);

            size_t initIndex = 0;
            Result<> initResult;
            for (const size_t count = m_modules.size(); initIndex < count; ++initIndex)
            {
                if (!(initResult = m_modules[initIndex].module->moduleInit()))
                {
                    break;
                }
            }

            if (!initResult)
            {
                for (size_t shutdownIndex = 0; shutdownIndex <= initIndex; ++shutdownIndex)
                {
                    m_modules[shutdownIndex].module->moduleShutdown();
                }

                return initResult;
            }

            m_nextPhase = ModulesPhase::PostInit;
            return ResultSuccess;
        }

        Result<> postInitModules()
        {
            const std::lock_guard lock(m_mutex);
            MY_DEBUG_ASSERT(m_nextPhase == ModulesPhase::PostInit);


            size_t initIndex = 0;
            Result<> initResult;

            for (const size_t count = m_modules.size(); initIndex < count; ++initIndex)
            {
                if (!(initResult = m_modules[initIndex].module->modulePostInit()))
                {
                    break;
                }
            }

            if (!initResult)
            {
                for (size_t shutdownIndex = 0; shutdownIndex <= initIndex; ++shutdownIndex)
                {
                    m_modules[shutdownIndex].module->moduleShutdown();
                }

                return initResult;
            }

            m_nextPhase = ModulesPhase::Shutdown;
            return ResultSuccess;
        }

        void shutdownModules()
        {
            const std::lock_guard lock(m_mutex);
            if (m_nextPhase != ModulesPhase::Shutdown)
            {
                return;
            }
            scope_on_leave
            {
                m_modules.clear();
                m_nextPhase = ModulesPhase::Init;
            };

            for (const ModuleEntry& entry : m_modules)
            {
                entry.module->moduleShutdown();
            }
        }

        std::vector<ModuleEntry> m_modules;
        std::mutex m_mutex;
        ModulesPhase m_nextPhase = ModulesPhase::Init;

        friend ModuleManager& getModuleManager();
        friend bool hasModuleManager();
    };

    ModuleManagerPtr createModuleManager()
    {
        return std::make_unique<ModuleManagerImpl>();
    }

    ModuleManager& getModuleManager()
    {
        MY_FATAL(ModuleManagerImpl::s_instance != nullptr);
        return *ModuleManagerImpl::s_instance;
    }

    bool hasModuleManager()
    {
        return ModuleManagerImpl::s_instance != nullptr;
    }

}  // namespace my
