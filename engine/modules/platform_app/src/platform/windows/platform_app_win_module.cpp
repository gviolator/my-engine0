// #my_engine_source_file

#include "my/module/module_api.h"
#include "my/rtti/rtti_impl.h"

namespace my
{
    class PlatformAppWinModule : public IModule
    {
        MY_RTTI_CLASS(my::PlatformAppWinModule, IModule)

    private:
        std::string getModuleName() override
        {
            return "Platform app (win)";
        }

        void moduleInit() override
        {
        }

        void modulePostInit() override
        {
        }

        void moduleShutdown() override
        {
        }
    };
}  // namespace my

MY_DECLARE_MODULE(my::PlatformAppWinModule)
