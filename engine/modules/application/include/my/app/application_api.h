// #my_engine_source_file

#pragma once

#include "my/rtti/rtti_object.h"
#include "my/utils/result.h"
#include "my/utils/runtime_enum.h"

namespace my
{
    MY_DEFINE_ENUM_(AppState,
                    NotStarted,
                    Initializing,
                    Invalid,
                    Active,
                    ShutdownRequested,
                    GameShutdownProcessed,
                    RuntimeShutdownProcessed,
                    ShutdownCompleted);

    struct MY_ABSTRACT_TYPE Application : IRttiObject
    {
        MY_INTERFACE(my::Application, IRttiObject)

        virtual AppState getState() const = 0;

        virtual bool isMainThread() const = 0;

        virtual Result<> startupOnCurrentThread() = 0;

        virtual bool step() = 0;

        virtual void stop() = 0;
    };

    MY_APPMODULE_EXPORT bool applicationExists();

    MY_APPMODULE_EXPORT Application& getApplication();

    namespace app_detail
    {
        MY_APPMODULE_EXPORT void setApplicationInstance(Application&);
        MY_APPMODULE_EXPORT Application* resetApplicationInstance();
    }  // namespace app_detail
}  // namespace my
