#pragma once
#include <string>
#include <string_view>
#include <vector>

#include "lua_toolkit/lua_interop.h"
#include "lua_toolkit/lua_utils.h"
#include "my/diag/assert.h"
#include "my/io/file_system.h"
#include "my/io/stream.h"
#include "my/runtime/disposable.h"
#include "my/service/service.h"
#include "my/rtti/rtti_impl.h"

namespace my
{
    class LuaScriptManager : public IServiceInitialization,
                             public IDisposable

    {
    public:
        MY_RTTI_CLASS(LuaScriptManager, IServiceInitialization, IDisposable)

        void addScriptSearchPath(io::FsPath path);
        void addScriptFileExtension(std::string_view ext);

        Result<> executeFileInternal(const io::FsPath& filePath);

        lua_State* getLua() const
        {
            return m_lua;
        }

    private:
        static int luaRequire(lua_State* l) noexcept;

        async::Task<> preInitService() override;
        void dispose() override;

        

        lua_State* m_lua = nullptr;
        std::vector<io::FsPath> m_searchPaths;
        std::string m_scriptFileExtension = ".lua";
    };
}  // namespace my