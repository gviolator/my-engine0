// #my_engine_source_file
#include "lua_script_manager.h"

#include "my/service/service_provider.h"

namespace my::script
{
    Result<io::FsPath> LuaScriptManager::resolveScriptPath(io::FsPath filePath)
    {
        io::FileSystem& fs = getServiceProvider().get<io::FileSystem>();
        io::FsPath scriptFilePath;
        if (filePath.isAbsolute())
        {
            scriptFilePath = filePath;
        }
        else
        {
            for (const auto& scriptsRoot : m_searchPaths)
            {
                io::FsPath modulePath = (scriptsRoot / filePath);
                modulePath = modulePath + m_scriptFileExtension;

                if (fs.exists(modulePath, io::FsEntryKind::File))
                {
                    scriptFilePath = std::move(modulePath);
                    break;
                }
            }
        }

        if (scriptFilePath.isEmpty())
        {
            return MakeError("Script file path not resolved:({})", filePath.getString());
        }

        if (!fs.exists(scriptFilePath, io::FsEntryKind::File))
        {
            return MakeError("Path is not regular file:({})", filePath.getString());
        }

        return scriptFilePath;
    }


    // Result<io::FilePtr> LuaScriptManager::openScriptFile(const io::FsPath& filePath) const
    // {
     
    //     io::FilePtr file = fs.openFile(scriptFilePath, io::AccessMode::Read, io::OpenFileMode::OpenExisting);
    //     if (!file)
    //     {
    //         return MakeError("Fail to open script file:({})", scriptFilePath.getString());
    //     }
    //     return file;
    // }

    // RealmPtr LuaScriptManager::createRealm()
    // {
    //     return nullptr;
    // }

    void LuaScriptManager::addScriptSearchPath(io::FsPath path)
    {
        m_searchPaths.emplace_back(std::move(path));
    }

    void LuaScriptManager::addScriptFileExtension(std::string_view ext)
    {
        m_scriptFileExtension = ext;
    }
}  // namespace my::script
