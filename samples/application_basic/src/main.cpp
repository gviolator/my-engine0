#include "my/app/application.h"
#include "my/app/application_delegate.h"
#include "my/diag/log_sinks.h"
#include "my/memory/runtime_stack.h"

using namespace my;
using namespace my::my_literals;
using namespace std::chrono_literals;

namespace my
{
    class SampleAppDelegate : public ApplicationInitDelegate
    {
        MY_RTTI_CLASS(SampleAppDelegate, ApplicationInitDelegate)

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

}  // namespace my

int main()
{
    rtstack_init(2_Mb);
    ApplicationPtr app = createApplication(std::make_unique<SampleAppDelegate>());
    app->startupOnCurrentThread().ignore();

    mylog_debug("Application started");

    []() -> async::Task<>
    {
        co_await 200ms;
        getApplication().stop();
        mylog("Request stop application");
    }().detach();

    while (app->step())
    {
        std::this_thread::sleep_for(50ms);
    }
    mylog("Application done...");

    return 0;
}
