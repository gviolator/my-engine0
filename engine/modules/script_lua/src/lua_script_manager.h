// #my_engine_source_file
#pragma once
#include "my/script/script_manager.h"
#include "my/rtti/rtti_impl.h"
#include "my/io/file_system.h"

namespace my::script
{
    class LuaScriptManager final : public ScriptManager, public IRttiObject
    {
        MY_RTTI_CLASS(my::script::LuaScriptManager, ScriptManager)

    public:
        //Result<io::FilePtr> openScriptFile(const io::FsPath& path) const;
        Result<io::FsPath> resolveScriptPath(io::FsPath path) override;

    private:
        //RealmPtr createRealm() override;
        void addScriptSearchPath(io::FsPath path) override;
        void addScriptFileExtension(std::string_view ext) override;

        std::vector<io::FsPath> m_searchPaths;
        std::string m_scriptFileExtension = ".lua";
    };
}
