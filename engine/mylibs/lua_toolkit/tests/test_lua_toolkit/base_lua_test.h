// #my_engine_source_file
#include "lua_toolkit/lua_headers.h"
#include "lua_toolkit/lua_interop.h"
#include "lua_toolkit/lua_utils.h"

namespace my::test
{
    class BaseLuaTest
    {
    protected:
        BaseLuaTest() :
            m_lua(createLuaState())
        {
            MY_DEBUG_ASSERT(m_lua);
        }

        ~BaseLuaTest()
        {
            lua_close(m_lua);
        }

        lua_State* getLua() const
        {
            return m_lua;
        }

        void executeProgram(std::string_view program, int expectedResultCount = 1) const
        {
            const int top = lua_gettop(getLua());

            if (Result<> res = lua::execute(m_lua, program, expectedResultCount, "test"); !res)
            {
                FAIL() << res.getError()->getDiagMessage();
            }

            if (expectedResultCount != LUA_MULTRET)
            {
                const int newTop = lua_gettop(getLua());
                const int resCount = newTop - top;
                if (resCount != expectedResultCount)
                {
                    FAIL() << "Expected (" << expectedResultCount << ") result, but stack diff (" << resCount << ")";
                    //return testing::AssertionFailure() 
                }
            }
            

            // if (auto res = lua::load(m_lua, program, "default_chunk"); !res)
            // {
            //     //MY_DEBUG_FATAL_FAILURE("Fail to load lua ({}):({})",  program, res.getError()->getDiagMessage());
            //     return testing::AssertionFailure() << "Fail to load lua program:" << program << ":" << res.getError()->getDiagMessage();
            // }
            // // expected that loaded chunk on stack
            // if (lua_pcall(m_lua, 0, expectedResultCount, 0) != 0)
            // {
            //     size_t len;
            //     const char* const message = lua_tolstring(m_lua, -1, &len);
            //     return testing::AssertionFailure() << "Fail to call:" << program << ":" << std::string_view{message, len};
            // }
            // lua_call must pop chunk's function from stack
            
            
            //return testing::AssertionSuccess();
        }

        testing::AssertionResult call(const char* name, int expectedResultCount = 1) const
        {
            const int top = lua_gettop(getLua());
            lua_getglobal(m_lua, name);
            if (lua_type(m_lua, -1) != LUA_TFUNCTION)
            {
                return testing::AssertionFailure() << "Pushed (" << name << ") not a function";
            }

            if (lua_pcall(m_lua, 0, 1, 0) != 0)
            {
                size_t len;
                const char* const message = lua_tolstring(m_lua, -1, &len);
                return testing::AssertionFailure() << "Fail to call:" << name << ":" << std::string_view{message, len};
            }

            // lua_call must pop chunk's function from stack
            const int newTop = lua_gettop(getLua());
            const int resCount = newTop - top;
            if (resCount != expectedResultCount)
            {
                return testing::AssertionFailure() << "Expected (" << expectedResultCount << ") result, but stack diff (" << resCount << ")";
            }

            return testing::AssertionSuccess();
        }

    private:
        static lua_State* createLuaState()
        {
            auto luaAlloc = [](void* ud, void* ptr, size_t osize, size_t nsize) -> void*
            {
                if (nsize == 0)
                {
                    ::free(ptr);
                    return nullptr;
                }
                return ::realloc(ptr, nsize);
            };

            auto* const luaState = lua_newstate(luaAlloc, nullptr);
            luaL_openlibs(luaState);

            return luaState;
        }

        lua_State* const m_lua;
    };

}  // namespace my::test
