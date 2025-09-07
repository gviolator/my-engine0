#include "script_manager.h"

#include "my/app/property_container.h"
#include "my/service/service_provider.h"

namespace my
{
    namespace
    {
        struct LuaChunkStreamLoader
        {
            std::array<std::byte, 512> buffer;
            io::IStream& streamReader;

            LuaChunkStreamLoader(io::IStream& inStreamReader) :
                streamReader(inStreamReader)
            {
            }

            static const char* read([[maybe_unused]] lua_State* lua, void* data, size_t* size) noexcept
            {
                auto& self = *reinterpret_cast<LuaChunkStreamLoader*>(data);

                Result<size_t> readResult = self.streamReader.read(self.buffer.data(), self.buffer.size());
                if (!readResult)
                {
                    MY_FAILURE("Fail to read input stream: ({})", readResult.getError()->getMessage());
                    *size = 0;
                    return nullptr;
                }

                *size = *readResult;
                return *size > 0 ? reinterpret_cast<const char*>(self.buffer.data()) : nullptr;
            }
        };

        struct ScriptsGlobalConfig
        {
            std::vector<io::FsPath> searchPaths;

            MY_CLASS_FIELDS(
                CLASS_FIELD(searchPaths))
        };

    }  // namespace

    int LuaScriptManager::luaRequire(lua_State* l) noexcept
    {
        const auto selfUpvalueIndex = lua_upvalueindex(1);
        MY_FATAL(lua_type(l, selfUpvalueIndex) == LUA_TLIGHTUSERDATA);
        LuaScriptManager* const self = reinterpret_cast<LuaScriptManager*>(lua_touserdata(l, selfUpvalueIndex));

        const int top = lua_gettop(l);
        io::FsPath filePath = *lua::cast<std::string>(l, -1);

        // executeFileInternal will keeps result on stack
        Result<> executeFileResult = self->executeFileInternal(filePath);

        if (executeFileResult)
        {
            const int top2 = lua_gettop(l);
            MY_DEBUG_ASSERT(top2 >= top);
            return top2 - top;
        }

        mylog_error("Script module ({}) execution error: {}", filePath.getString(), executeFileResult.getError()->getMessage());
        return 0;
    }

    async::Task<> LuaScriptManager::preInitService()
    {
        auto luaAlloc = []([[maybe_unused]] void* ud, void* ptr, [[maybe_unused]] size_t osize, size_t nsize) noexcept -> void*
        {
            // TODO: replace with custom allocator
            if (nsize == 0)
            {
                ::free(ptr);
                return nullptr;
            }

            void* const memPtr = ::realloc(ptr, nsize);
            MY_FATAL(memPtr, "Fail to allocate/reallocate script memory:({}) bytes", nsize);
            return memPtr;
        };

        if (std::optional<ScriptsGlobalConfig> config = getServiceProvider().get<PropertyContainer>().getValue<ScriptsGlobalConfig>("/scripts"))
        {
            for (auto& path : config->searchPaths)
            {
                addScriptSearchPath(std::move(path));
            }
        }

        m_lua = lua_newstate(luaAlloc, nullptr);
        MY_FATAL(m_lua);
        luaL_openlibs(m_lua);

        lua_pushlightuserdata(m_lua, this);
        lua_pushcclosure(m_lua, luaRequire, 1);
        lua_setglobal(m_lua, "require");

        return async::Task<>::makeResolved();
    }

    void LuaScriptManager::dispose()
    {
        if (m_lua)
        {
            lua_close(m_lua);
        }
    }

    void LuaScriptManager::addScriptSearchPath(io::FsPath path)
    {
        m_searchPaths.emplace_back(std::move(path));
    }

    void LuaScriptManager::addScriptFileExtension(std::string_view ext)
    {
        m_scriptFileExtension = ext;
    }

    Result<> LuaScriptManager::executeFileInternal(const io::FsPath& filePath)
    {
        MY_DEBUG_FATAL(m_lua);

        io::FileSystem& fs = getServiceProvider().get<io::FileSystem>();
        io::FsPath moduleFullPath;
        if (filePath.isAbsolute())
        {
            moduleFullPath = filePath;
        }
        else
        {
            for (const auto& scriptsRoot : m_searchPaths)
            {
                io::FsPath modulePath = (scriptsRoot / filePath);
                modulePath = modulePath + m_scriptFileExtension;

                if (fs.exists(modulePath, io::FsEntryKind::File))
                {
                    moduleFullPath = std::move(modulePath);
                    break;
                }
            }
        }

        if (moduleFullPath.isEmpty() || !fs.exists(moduleFullPath, io::FsEntryKind::File))
        {
            return MakeError("Script file path not resolved:({})", filePath.getString());
        }

        auto file = fs.openFile(moduleFullPath, io::AccessMode::Read, io::OpenFileMode::OpenExisting);
        if (!file)
        {
            return MakeError("Fail to open script file:({})", moduleFullPath.getString());
        }

        io::StreamPtr stream = file->createStream();
        LuaChunkStreamLoader loader{*stream};

        if (lua_load(m_lua, LuaChunkStreamLoader::read, &loader, moduleFullPath.getString().c_str(), "t") == 0)
        {
            if (lua_pcall(m_lua, 0, LUA_MULTRET, 0) != 0)
            {
                auto err = *lua::cast<std::string>(m_lua, -1);
                return MakeError("Execution error: {}", err);
            }
        }
        else
        {
            auto err = *lua::cast<std::string>(m_lua, -1);
            return MakeError("Parse error: {}", err);
        }

        return ResultSuccess;
    }

}  // namespace my