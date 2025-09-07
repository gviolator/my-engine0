// #my_engine_source_file

#include "my/app/application.h"

#include "application_impl.h"
#include "logging_service.h"
#include "my/app/property_container.h"
#include "my/io/special_paths.h"
#include "my/io/virtual_file_system.h"
#include "my/service/service_provider.h"
#include "my/utils/string_conv.h"

namespace my
{
    Result<> initAndApplyConfiguration(ApplicationInitDelegate*);

    Result<> setupCoreServicesAndConfigure(ApplicationInitDelegate* appDelegate)
    {
        std::unique_ptr<LoggingService> loggingService = std::make_unique<LoggingService>();

        setDefaultServiceProvider(createServiceProvider());

        ServiceProvider& serviceProvider = getServiceProvider();

        serviceProvider.addService(std::move(loggingService));
        serviceProvider.addService(createPropertyContainer());
        serviceProvider.addService(io::createVirtualFileSystem());

        return initAndApplyConfiguration(appDelegate);
    }

    Result<> initAndApplyConfiguration(ApplicationInitDelegate* appDelegate)
    {
        namespace fs = std::filesystem;

        auto& globalProps = getServiceProvider().get<PropertyContainer>();
        globalProps.addVariableResolver("folder", [](std::string_view folderStr) -> std::optional<std::string>
        {
            io::KnownFolder folder = io::KnownFolder::Current;
            if (!parse(folderStr, folder))
            {
                MY_FAILURE("Bad known_folder value ({})", folderStr);
                return "BAD_FOLDER";
            }

            const fs::path folderPath = io::getKnownFolderPath(folder);
            MY_DEBUG_ASSERT(!folderPath.empty());

            const std::wstring wcsPath = folderPath.wstring();
            return strings::wstringToUtf8(wcsPath);
        });

        if (appDelegate)
        {
            CheckResult(appDelegate->configureApplication());
        }

        return ResultSuccess;
    }

    void shutdownCoreServices()
    {
    }

    ApplicationPtr createApplication(ApplicationInitDelegatePtr appDelegate)
    {
        MY_FATAL(!applicationExists());

        if (!setupCoreServicesAndConfigure(appDelegate.get()))
        {
            return nullptr;
        }

        ApplicationPtr app = std::make_unique<ApplicationImpl>(shutdownCoreServices);
        MY_FATAL(app);

        if (appDelegate && !appDelegate->registerApplicationServices())
        {
            return nullptr;
        }

        return app;
    }
}  // namespace my
