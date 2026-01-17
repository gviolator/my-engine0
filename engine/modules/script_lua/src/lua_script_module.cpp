// #my_engine_source_file

#include "lua_realm.h"
#include "lua_script_manager.h"
#include "my/module/module_api.h"
#include "my/rtti/rtti_impl.h"

namespace my::script
{
    class LuaScriptModule final : public ModuleBase
    {
        MY_RTTI_CLASS(my::script::LuaScriptModule, IModule)

        Result<> moduleInit() override
        {
            MY_MODULE_EXPORT_SERVICE(LuaScriptManager);
            MY_MODULE_EXPORT_CLASS(LuaRealm);
            return kResultSuccess;
        }
    };
}  // namespace my::script

MY_DECLARE_MODULE(my::script::LuaScriptModule)
