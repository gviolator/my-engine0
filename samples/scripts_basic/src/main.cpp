#include "my/app/application.h"
#include "my/app/application_delegate.h"
#include "my/diag/log_sinks.h"
#include "my/io/virtual_file_system.h"
#include "my/memory/runtime_stack.h"
#include "my/script/script_manager.h"
#include "my/service/service_provider.h"
#include "my/script/realm.h"
#include "my/dispatch/closure_value.h"

//extern "C"
//{
//#include <rpmalloc.h>
//}

using namespace my;
using namespace my::my_literals;
using namespace std::chrono_literals;
using namespace std::chrono;

namespace my
{
    class SampleAppDelegate : public ApplicationInitDelegate
    {
        MY_REFCOUNTED_CLASS(SampleAppDelegate, ApplicationInitDelegate)

        Result<> configureApplication() override
        {
            // m_sink = diag::getDefaultLogger().addSink(diag::createConsoleSink());

            return ResultSuccess;
        }

        Result<> registerApplicationServices() override
        {
            return ResultSuccess;
        }

        diag::LogSinkEntry m_sink;
    };

    script::RealmPtr createRealm()
    {
        ClassDescriptorPtr klass = getServiceProvider().findClass<script::Realm>();
        if (!klass)
        {
            return nullptr;
        }

        UniPtr<IRttiObject> ptr = *klass->getConstructor()->invoke(nullptr, {});
        return ptr.release<script::RealmPtr>();
        // return nullptr;
    }

}  // namespace my

Result<> setupVfs()
{
    io::VirtualFileSystem& fs = getServiceProvider().get<io::VirtualFileSystem>();

    CheckResult(fs.mount("/content", io::createNativeFileSystem("c:\\proj\\my-engine0\\build\\core_content_cache")));
    //fs.mount("/content", io::createNativeFileSystem("c:\\proj\\my-engine0\\build\\content_shared_cache")).ignore();


    return ResultSuccess;
}


Result<> setupCoreScripts()
{
    script::ScriptManager& manager = getServiceProvider().get<script::ScriptManager>();
    manager.addScriptSearchPath("/content/prog");

    return ResultSuccess;
}

int main()
{
    // {
    //     rpmalloc_initialize(nullptr);

    //     void* mem = rpmalloc(128);

    //     mem = rprealloc(mem, 256);

    //     rpfree(mem);

    //     rpmalloc_finalize();

    // }

    rtstack_init(2_Mb);
    ApplicationPtr app = createApplication(rtti::createInstance<SampleAppDelegate>());
    app->startupOnCurrentThread().ignore();
    setupVfs().ignore();

    if (auto res = setupCoreScripts(); !res)
    {
        MY_FATAL_FAILURE("Fail to start:({})", res.getError()->getMessage());
    }

    script::RealmPtr realm = createRealm();
    Ptr<ClosureValue> mainStep = EXPR_Block -> Ptr<ClosureValue>
    {
        Ptr<ReadonlyDictionary> interopModule = *realm->executeFile("interop");
        return interopModule->getValue("interop_appMainStep");
    };


    mylog_debug("Application started");

    //[]() -> async::Task<>
    //{
    //    co_await 10ms;


    //    getApplication().stop();
    //    mylog("Request stop application");
    //}().detach();


    using TimePoint = system_clock::time_point;
    TimePoint t0 = system_clock::now();
    
    while (app->step())
    {
        rtstack_scope;
        const script::InvocationScopeGuard iGuard{*realm};

        if (app->getState() == AppState::Active)
        {
            const TimePoint t1 = system_clock::now();
            const auto dt = duration_cast<milliseconds>(t1 - t0).count();
            const float fDt = static_cast<float>(dt) / 1000.f;

            Ptr<BooleanValue> continueGame = *mainStep->invoke({makeValueCopy(fDt)});
            if (!continueGame->getBool())
            {
                getApplication().stop();
                //break;
            }
            std::this_thread::sleep_for(50ms);
        }
    }
    mylog("Application done...");

    return 0;
}
