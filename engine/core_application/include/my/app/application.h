// #my_engine_source_file

#pragma once
#include <memory>

#include "my/app/application_api.h"
#include "my/app/application_delegate.h"

namespace my
{
    using ApplicationPtr = std::unique_ptr<Application>;

    ApplicationPtr createApplication(ApplicationInitDelegatePtr appDelegate);
}
