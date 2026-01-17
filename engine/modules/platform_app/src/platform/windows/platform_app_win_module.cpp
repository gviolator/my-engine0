// #my_engine_source_file

#include "my/module/module_api.h"
#include "my/rtti/rtti_impl.h"

namespace my
{
    class PlatformAppWinModule : public ModuleBase
    {
        MY_RTTI_CLASS(my::PlatformAppWinModule, IModule)

        Result<> moduleInit() override
        {
            return kResultSuccess;
        }

    };
}  // namespace my

MY_DECLARE_MODULE(my::PlatformAppWinModule)
