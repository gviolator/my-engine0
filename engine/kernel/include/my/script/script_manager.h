// #my_engine_source_file
#pragma once

#include "my/io/fs_path.h"
#include "my/rtti/type_info.h"

#include <string_view>

// #include "my/script/realm.h"
#include "my/utils/result.h"

namespace my::script {
    
struct MY_ABSTRACT_TYPE IScriptManager
{
    MY_TYPEID(my::script::IScriptManager)

    virtual ~IScriptManager() = default;

    virtual void AddSearchPath(io::FsPath path) = 0;

    virtual void AddFileExtension(std::string_view ext) = 0;

    virtual Result<io::FsPath> ResolvePath(io::FsPath path) = 0;

    virtual void EnableDebug(bool enableDebug) = 0;
};
}  // namespace my::script
