// #my_engine_source_file

#pragma once

#include <memory>
#include <string>

#include "my/rtti/rtti_object.h"

namespace my
{

    struct MY_ABSTRACT_TYPE IModule : IRttiObject
    {
        virtual ~IModule() = default;

        virtual std::string getModuleName() = 0;  // could be usefull for Editor

        virtual void moduleInit() = 0;

        virtual void modulePostInit() = 0;

        virtual void moduleShutdown() = 0;
    };

    class MY_ABSTRACT_TYPE ModuleBase : public IModule
    {
        MY_TYPEID(my::ModuleBase)
        MY_CLASS_BASE(IModule)

    public:
        void modulePostInit() override
        {
        }

        void moduleShutdown() override
        {
        }
    };

    using ModulePtr = std::unique_ptr<IModule>;

}  // namespace my
