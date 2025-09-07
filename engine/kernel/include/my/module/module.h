// #my_engine_source_file
#pragma once

#include <memory>
#include <string>

#include "my/rtti/rtti_object.h"
#include "my/utils/result.h"

namespace my
{

    struct MY_ABSTRACT_TYPE IModule : IRttiObject
    {
        virtual ~IModule() = default;

        //virtual std::string getModuleDescription() = 0;  // could be usefull for Editor

        virtual Result<> moduleInit() = 0;

        virtual Result<> modulePostInit() = 0;

        virtual void moduleShutdown() = 0;
    };

    class MY_ABSTRACT_TYPE ModuleBase : public IModule
    {
        MY_TYPEID(my::ModuleBase)
        MY_CLASS_BASE(IModule)

    public:
        Result<> modulePostInit() override
        {
            return ResultSuccess;
        }

        void moduleShutdown() override
        {
        }
    };

    using ModulePtr = std::unique_ptr<IModule>;

}  // namespace my
