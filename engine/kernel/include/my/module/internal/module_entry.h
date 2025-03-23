// nau/module/internal/module_entry.h
//
// Copyright (c) N-GINN LLC., 2023-2025. All rights reserved.
//
#pragma once

#include "EASTL/string.h"
#include "my/io/file_system.h"
#include "my/kernel/kernel_config.h"


struct IModule;

namespace my
{
    struct ModuleEntry
    {
        my::string name;
        std::shared_ptr<::my::IModule> iModule = nullptr;
        bool isModuleInitialized = false;

#if !MY_STATIC_RUNTIME
        std::wstring dllPath;
        HMODULE dllHandle = 0;
#endif
    };

}  // namespace my
