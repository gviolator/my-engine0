// #my_engine_source_file
#pragma once

#include <string_view>

#include "my/io/fs_path.h"
#include "my/rtti/type_info.h"
// #include "my/script/realm.h"
#include "my/utils/result.h"

namespace my::script
{
    struct MY_ABSTRACT_TYPE ScriptManager
    {
        MY_TYPEID(my::script::ScriptManager)

        virtual ~ScriptManager() = default;

        virtual void addScriptSearchPath(io::FsPath path) = 0;

        virtual void addScriptFileExtension(std::string_view ext) = 0;

        virtual Result<io::FsPath> resolveScriptPath(io::FsPath path) = 0;
    };
}  // namespace my::script
