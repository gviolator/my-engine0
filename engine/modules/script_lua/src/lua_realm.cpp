// #my_engine_source_file
#include "lua_realm.h"

// #include "helpers/chunk_loader.h"
#include "lua_script_manager.h"
#include "lua_toolkit/lua_utils.h"
#include "my/io/file_system.h"
#include "my/io/memory_stream.h"
#include "my/memory/runtime_stack.h"
#include "my/service/service_provider.h"

using namespace my::io;

namespace my::script
{
    namespace
    {
        inline lua::ValueKeepMode getKeepModeFromOpts(InvokeOptsFlag opts) noexcept
        {
            return opts.isSet(InvokeOpts::KeepResult) ? lua::ValueKeepMode::AsReference : lua::ValueKeepMode::OnStack;
        }
    }  // namespace

    class LuaRealm::InvocationGuardImpl final : public script_detail::InvocationGuardHandle
    {
    public:
        InvocationGuardImpl(LuaRealm& realm, InvokeOptsFlag opts) :
            m_realm(realm),
            m_valueKeeperGuard(realm.m_lua, getKeepModeFromOpts(opts)),
            m_parentGuard(realm.m_currentInvocationGuard),
            m_opts(opts),
            m_stackTop(lua_gettop(m_realm.m_lua))
        {
            m_realm.m_currentInvocationGuard = this;
        }

        ~InvocationGuardImpl()
        {
            MY_DEBUG_ASSERT(m_realm.m_currentInvocationGuard == this, "InvocationScopeGuard scope is broken");
            m_realm.m_currentInvocationGuard = m_parentGuard;
        }

        void* operator new(size_t size)
        {
            void* const mem = getRtStackAllocator().alloc(size);
            MY_DEBUG_FATAL(reinterpret_cast<uintptr_t>(mem) % alignof(InvocationGuardImpl) == 0);

            return mem;
        }

        void operator delete(void* ptr, size_t)
        {
            getRtStackAllocator().free(ptr);
        }

        int getOriginStackTop() const
        {
            return m_stackTop;
        }

        std::optional<InvokeOptsFlag> getOpts() const
        {
            return m_opts;
        }

    private:
        LuaRealm& m_realm;
        const lua::ValueKeeperGuard m_valueKeeperGuard;
        const InvocationGuardImpl* m_parentGuard = nullptr;
        const std::optional<InvokeOptsFlag> m_opts;
        const int m_stackTop;
    };

    int LuaRealm::resolveImportPath(lua_State* l) noexcept
    {
        FsPath importPath = lua::toStringView(l, -1);

        Result<FsPath> resolvedPathResult = getServiceProvider().get<LuaScriptManager>().resolveScriptPath(importPath);
        Lua_CheckResult(l, resolvedPathResult);
        lua_pushstring(l, resolvedPathResult->getCStr());

        return 1;
    }

    int LuaRealm::loadImport(lua_State* l) noexcept
    {
        // const auto selfUpvalueIndex = lua_upvalueindex(1);
        // MY_FATAL(lua_type(l, selfUpvalueIndex) == LUA_TLIGHTUSERDATA);
        // // LuaRealm* const realm = reinterpret_cast<LuaRealm*>(lua_touserdata(l, selfUpvalueIndex));

        const int top = lua_gettop(l);
        FsPath filePath = lua::toStringView(l, -1);

        FileSystem& fs = getServiceProvider().get<FileSystem>();
        FilePtr file = fs.openFile(filePath, AccessMode::Read, OpenFileMode::OpenExisting);
        MY_DEBUG_ASSERT(file);
        if (!file || !file->isOpened())
        {
            Lua_ThrowError(l, std::format("Fail to open program file: {}", filePath.getString()));
        }

        io::StreamPtr stream = file->createStream();
        Result<> execResult = lua::execute(l, *stream, LUA_MULTRET, filePath.getCStr());
        Lua_CheckResult(l, execResult);

        const int top2 = lua_gettop(l);
        MY_DEBUG_ASSERT(top2 >= top);
        return top2 - top;
    }

    LuaRealm::LuaRealm()
    {
        auto luaAlloc = []([[maybe_unused]] void* ud, void* ptr, [[maybe_unused]] size_t osize, size_t nsize) noexcept -> void*
        {
            if (nsize == 0)
            {
                ::free(ptr);
                return nullptr;
            }

            void* const memPtr = ::realloc(ptr, nsize);
            MY_FATAL(memPtr, "Fail to allocate/reallocate script memory:({}) bytes", nsize);
            return memPtr;
        };

        constexpr std::string_view program = R"--(
            __My_ImportedModules = {}

            function require(importFilePath)
                local resolvedPath = __myResolveImportPath(importFilePath)
                local moduleExport = __My_ImportedModules[resolvedPath]

                if moduleExport == nil then
                    moduleExport = __myLoadImportedModule(resolvedPath)
                    __My_ImportedModules[resolvedPath] = moduleExport
                end

                return moduleExport
            end
        )--";

        m_lua = lua_newstate(luaAlloc, nullptr);
        MY_FATAL(m_lua);
        luaL_openlibs(m_lua);

        assert_lstack_unchanged(m_lua);

        lua_pushcclosure(m_lua, resolveImportPath, 0);
        lua_setglobal(m_lua, "__myResolveImportPath");

        lua_pushcclosure(m_lua, loadImport, 0);
        lua_setglobal(m_lua, "__myLoadImportedModule");

        lua::execute(m_lua, program, LUA_MULTRET, "internalSetup").ignore();

        /*
                lua_pushlightuserdata(m_lua, this);
                lua_pushcclosure(m_lua, luaRequire, 1);
                lua_setglobal(m_lua, "require");
        */
    }

    LuaRealm::~LuaRealm()
    {
        if (m_lua)
        {
            lua_close(m_lua);
        }
    }

    // Result<> LuaRealm::executeFileInternal(const io::FsPath& filePath)
    // {
    //     MY_DEBUG_FATAL(m_lua);

    //     Result<io::FilePtr> file = getServiceProvider().get<LuaScriptManager>().openScriptFile(filePath);
    //     CheckResult(file);
    //     io::StreamPtr stream = (*file)->createStream();

    //     rtstack_scope;

    //     return execStream

    //     ChunkLoader loader{*stream, getRtStackAllocator(), 1024};

    //     const std::string fullFilePath = (*file)->getPath().getString();

    //     if (lua_load(m_lua, ChunkLoader::read, &loader, fullFilePath.c_str(), "t") == 0)
    //     {
    //         if (lua_pcall(m_lua, 0, LUA_MULTRET, 0) != 0)
    //         {
    //             auto err = *lua::cast<std::string>(m_lua, -1);
    //             return MakeError("Execution error: {}", err);
    //         }
    //     }
    //     else
    //     {
    //         auto err = *lua::cast<std::string>(m_lua, -1);
    //         return MakeError("Parse error: {}", err);
    //     }

    //     return ResultSuccess;
    // }

    template <typename Call>
    InvokeResult LuaRealm::executeImpl(Call call)
    {
        static_assert(std::is_invocable_r_v<Result<>, Call>);

        MY_DEBUG_FATAL(m_lua);

        const int top = lua_gettop(m_lua);
        CheckResult(call());

        const int top2 = lua_gettop(m_lua);
        MY_DEBUG_ASSERT(top <= top2);

        const int resCount = top2 - top;
        if (resCount == 0)
        {
            return nullptr;
        }

        MY_DEBUG_ASSERT(resCount == 1, "Multi result is not supported (yet)");
        if (resCount != 1)
        {
            lua_settop(m_lua, top);
            return MakeError("Function return multi result which is not supported");
        }

        auto [value, keepValueOnStack] = lua::makeValueFromLuaStack(m_lua, -1);
        if (!keepValueOnStack)
        {
            lua_settop(m_lua, top);
        }

        return InvokeResult{std::move(value)};
    }

    InvokeResult LuaRealm::execute(std::span<const std::byte> program, const char* chunkName)
    {
        // MY_DEBUG_FATAL(m_lua);
        if (program.empty())
        {
            return MakeError("empty program");
        }

        return executeImpl([this, &program, chunkName]
        {
            rtstack_scope;
            io::MemoryStreamPtr stream = io::createReadonlyMemoryStream(program, getRtStackAllocatorPtr());
            return lua::execute(m_lua, *stream, LUA_MULTRET, chunkName);
        });
    }

    InvokeResult LuaRealm::executeFile(const io::FsPath& path)
    {
        // MY_DEBUG_FATAL(m_lua);
        if (path.isEmpty())
        {
            return MakeError("empty program");
        }

        return executeImpl([this, &path]() -> Result<>
        {
            lua_getglobal(m_lua, "require");
            lua_pushstring(m_lua, path.getCStr());
            Lua_CheckErr(m_lua, lua_pcall(m_lua, 1, LUA_MULTRET, 0));

            return ResultSuccess;
        });
    }

    Result<> LuaRealm::compile(io::IStream& programSource, io::IStream& compilationOutput)
    {
        return MakeError("Not impl");
    }

    void LuaRealm::registerClass(ClassDescriptorPtr classDescriptor)
    {
    }
    /*
        InvokeResult LuaRealm::invokeGlobal(const char* callableName, DispatchArguments args, InvokeOptsFlag opts)
        {
            MY_DEBUG_ASSERT(!opts, "Invoke options is not supported (yet)");
            MY_DEBUG_FATAL(m_lua != nullptr);
            MY_DEBUG_ASSERT(callableName);

            MY_DEBUG_FATAL(m_currentInvocationGuard, "There is ni InvocationScopeGuard in current scope");

            const int type = lua_getglobal(m_lua, callableName);
            MY_DEBUG_ASSERT(type == LUA_TFUNCTION, "Global value ({}) is not a callable thing", callableName);

            if (type != LUA_TFUNCTION)
            {
                return MakeError("Global value ({}) is not a callable thing", callableName);
            }

            constexpr int MaxResultCount = 1;

            for (const Ptr<>& arg : args)
            {
                lua::pushRuntimeValue(m_lua, arg).ignore();
            }

            if (lua_pcall(m_lua, static_cast<int>(args.size()), MaxResultCount, 0) != 0)
            {
                if (lua_type(m_lua, -1) == LUA_TSTRING)
                {
                    std::string error = *lua::cast<std::string>(m_lua, -1);
                    return MakeError(std::move(error));
                }

                return MakeError("call error: unknown");
            }

            const int currentStackTop = lua_gettop(m_lua);
            const int originStackTop = m_currentInvocationGuard->getOriginStackTop();
            MY_DEBUG_ASSERT(originStackTop <= currentStackTop);

            if (originStackTop < currentStackTop)
            {
                return lua::makeValueFromLuaStack(m_lua, currentStackTop);
            }

            return nullptr;
        }
    */
    Result<std::unique_ptr<IDispatch>> LuaRealm::createClassInstance(std::string_view className)
    {
        return MakeError("Not Implmeneted");
    }

    script_detail::InvocationGuardHandlePtr LuaRealm::openInvocationScope(InvokeOptsFlag opts)
    {
        auto invocationGuard = std::make_unique<InvocationGuardImpl>(*this, opts);
#ifdef MY_DEBUG_ASSERT_ENABLED
        const InvocationGuardImpl* const expectedGuardPtrToCheck = invocationGuard.get();
        scope_on_leave
        {
            MY_DEBUG_ASSERT(m_currentInvocationGuard == expectedGuardPtrToCheck);
        };
#endif
        return invocationGuard;
    }

    // InvokeOptsFlag LuaRealm::getCurrentInvokeOpts(std::optional<InvokeOptsFlag> overrideOpts)
    // {
    //     if (overrideOpts)
    //     {
    //         return *overrideOpts;
    //     }

    //     if (const std::optional<InvokeOpts> opts = m_currentInvocationGuard ? m_currentInvocationGuard->getOpts() : std::nullopt)
    //     {
    //         return *opts;
    //     }

    //     return InvokeOpts::KeepResult;
    // }
}  // namespace my::script
