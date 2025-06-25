// #my_engine_source_file

#include "my/app/application_api.h"
#include "my/module/module_api.h"
#include "my/rtti/rtti_impl.h"

namespace my
{
    namespace
    {
        Application* g_Application = nullptr;
    }

    bool applicationExists()
    {
        return g_Application != nullptr;
    }

    Application& getApplication()
    {
        MY_FATAL(g_Application != nullptr);
        return *g_Application;
    }

}  // namespace my

namespace my::app_detail
{
    void setApplicationInstance(Application& app)
    {
        MY_FATAL(!applicationExists());
        g_Application = &app;
    }

    Application* resetApplicationInstance()
    {
        return std::exchange(g_Application, nullptr);
    }
}

// MY_DECLARE_MODULE(my::AppModule)

