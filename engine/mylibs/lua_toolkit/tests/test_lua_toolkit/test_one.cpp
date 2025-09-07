#if 0
#include <filesystem>

#include "lua_toolkit/lua_interop.h"
#include "lua_toolkit/lua_utils.h"
#include "my/dispatch/class_descriptor_builder.h"
#include "my/memory/bytes_buffer.h"
#include "my/utils/functor.h"
#include "my/utils/scope_guard.h"

namespace my::test
{
#pragma region Heleprs

    class Win32File
    {
    public:
        Win32File(const std::string& filename) :
            m_hFile{::CreateFileA(getFullpath(filename).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)}
        {
        }

        ~Win32File()
        {
            if(m_hFile != INVALID_HANDLE_VALUE)
            {
                ::CloseHandle(m_hFile);
            }
        }

        size_t read(std::byte* ptr, size_t size) const
        {
            MY_DEBUG_ASSERT(m_hFile != INVALID_HANDLE_VALUE);
            DWORD bytesRead = 0;
            const auto readOk = ::ReadFile(m_hFile, ptr, static_cast<DWORD>(size), &bytesRead, nullptr) == TRUE;

            return readOk ? static_cast<size_t>(bytesRead) : 0;
        }

        std::string readToString()
        {
            nau::BytesBuffer buffer{256};

            size_t offset = 0;

            for(size_t bytesRead = 0; (bytesRead = read(buffer.data() + offset, buffer.size() - offset)) > 0;)
            {
                offset += bytesRead;
                if(offset == buffer.size())
                {
                    buffer.resize(buffer.size() + 512);
                }
            }

            return std::string{reinterpret_cast<const char*>(buffer.data()), offset};
        }

        explicit operator bool() const noexcept
        {
            return m_hFile != INVALID_HANDLE_VALUE;
        }

    private:
        static std::string getFullpath(std::string_view path)
        {
            const auto p = s_dataRootPath / std::filesystem::path{path};

            return std::filesystem::absolute(p).string();
        }

        const HANDLE m_hFile = INVALID_HANDLE_VALUE;
        inline static const std::filesystem::path s_dataRootPath{L"C:\\proj\\NauPrototype\\engine\\libs\\lua_toolkit\\tests\\resources"};
    };

    struct LuaFileLoadState
    {
        const Win32File file;
        const nau::BytesBuffer buffer{256};

        LuaFileLoadState(const std::string filename) :
            file(filename)
        {
        }

        static const char* read([[maybe_unused]] lua_State* lua, void* data, size_t* size) noexcept
        {
            auto this_ = reinterpret_cast<LuaFileLoadState*>(data);

            *size = this_->file.read(this_->buffer.data(), this_->buffer.size());
            return *size > 0 ? reinterpret_cast<const char*>(this_->buffer.data()) : nullptr;
        }
    };

    inline nau::Result<> executeChunkFromFile(lua_State* l, const char* fileName, const char* chunkName = nullptr)
    {
        LuaFileLoadState loader(fileName);
        if(!loader.file)
        {
            return MakeError("File not valid: ({})", fileName);
        }

        if(lua_load(l, LuaFileLoadState::read, &loader, (chunkName ? chunkName : fileName), "t") == 0)
        {
            if(lua_pcall(l, 0, 0, 0) != 0)
            {
                auto err = lua::cast<std::string>(l, -1);

                // std::cout << std::format("Lua call error: {}", *lua::LuaCast<std::string>(l, -1));
                // return RtMakeError("Fail to execute: {}", *lua::LuaCast<std::string>(l, -1));
                MakeError("Fail to execute");
            }
        }
        else
        {
            // std::cout << std::format("Lua load error: {}", *lua::LuaCast<std::string>(l, -1));
            // return RtMakeError("Fail to read/parse: {}", *lua::LuaCast<std::string>(l, -1));
            MakeError("Fail to parse");
        }

        return {};
    }

    int luaRequire(lua_State* l) noexcept
    {
        std::filesystem::path modulePath = *lua::cast<std::string>(l, -1);
        modulePath.replace_extension(".lua");

        modulePath = std::filesystem::path{"build"} / modulePath;

        LuaFileLoadState loader(modulePath.string().c_str());
        if(!loader.file)
        {
            // return MakeError("File not valid: ({})", fileName);
            return 0;
        }

        const int top = lua_gettop(l);

        if(lua_load(l, LuaFileLoadState::read, &loader, modulePath.string().c_str(), "t") == 0)
        {
            if(lua_pcall(l, 0, LUA_MULTRET, 0) != 0)
            {
                auto err = lua::cast<std::string>(l, -1);

                // std::cout << std::format("Lua call error: {}", *lua::LuaCast<std::string>(l, -1));
                // return RtMakeError("Fail to execute: {}", *lua::LuaCast<std::string>(l, -1));
                MakeError("Fail to execute");
            }

            const int top2 = lua_gettop(l);
            MY_DEBUG_ASSERT(top2 >= top);

            return top2 - top;
        }
        else
        {
            // std::cout << std::format("Lua load error: {}", *lua::LuaCast<std::string>(l, -1));
            // return RtMakeError("Fail to read/parse: {}", *lua::LuaCast<std::string>(l, -1));
            MakeError("Fail to parse");
        }

        return 0;
    };

#pragma endregion

    struct IMyInterface1 : virtual IRefCounted
    {
        MY_INTERFACE(nau::test::IMyInterface1, IRefCounted)

#pragma region Class meta
        MY_REFCOUNED_CLASSMETHODS(
            CLASS_METHOD(IMyInterface1, sub1))
#pragma endregion

        virtual float sub1(float x, float y) = 0;
    };

    class MyService : public IMyInterface1
    {
    public:
        MY_REFCOUNED_CLASS(nau::test::MyService, IMyInterface1)

#pragma region Class meta
        MY_REFCOUNED_CLASSNAME("MyService");

        MY_REFCOUNED_CLASSMETHODS(
            CLASS_METHOD(MyService, add1),
            CLASS_METHOD(MyService, getStaticData))

        MY_REFCOUNED_CLASSFIELDS(
            CLASS_FIELD(m_mem))
#pragma endregion

    public:
        ~MyService()
        {
            std::cout << "Instance destructor\n";
        }

        static std::string getStaticData()
        {
            return "data";
        }

        std::string add1(float x, std::optional<float> y = std::nullopt)
        {
            const auto res = y ? (x + *y) : x;
            return std::format("C++ result ({})", res);
        }

        float sub1(float x, float y) override
        {
            return x - y;
        }

    private:
        float m_mem = 0;
    };

    class MyTestCallback : public IRefCounted
    {
        MY_REFCOUNED_CLASS(MyTestCallback, IRefCounted)

        MY_REFCOUNED_CLASSNAME("MyTestCallback")

        MY_REFCOUNED_CLASSMETHODS(
            CLASS_METHOD(MyTestCallback, subscribe),
            CLASS_METHOD(MyTestCallback, notify)
        )

    public:
        using Callback = Functor<unsigned(const std::string&)>;

        void subscribe(Callback cb)
        {
            m_callbacks.emplace_back(std::move(cb));
        }

        void notify(const std::string& text)
        {
            for (auto& cb : m_callbacks)
            {
                cb(text);
            }
        }

    private:

        std::vector<Callback> m_callbacks;
    };

    TEST(TestLua, Test1)
    {
        auto luaAlloc = [](void* ud, void* ptr, size_t osize, size_t nsize) -> void*
        {
            return ::realloc(ptr, nsize);
        };

        auto l = lua_newstate(luaAlloc, nullptr);
        scope_on_leave
        {
            lua_close(l);
        };

        luaL_openlibs(l);

        lua_pushcclosure(l, luaRequire, 0);
        lua_setglobal(l, "require");

        lua::initializeClass(l, getClassDescriptor<MyService>(), false).ignore();
        lua::initializeClass(l, getClassDescriptor<MyTestCallback>(), false).ignore();

        auto res = executeChunkFromFile(l, "build\\main.lua");
    }

    template <typename... M>
    inline static auto makeMethodInfos(const std::tuple<M...>& methodInfos)
    {
        return [&]<size_t... I>(std::index_sequence<I...>)
        {
            return std::tuple{std::get<I>(methodInfos)...};
        }(std::make_index_sequence<sizeof...(M)>{});

        // return MethodInfoImplTuple
    }

    TEST(TestLua, Test2)
    {
        auto classDesc = getClassDescriptor<MyService>();
        std::cout << std::format("Api count class:({}) [{}]\n", classDesc->getClassName(), classDesc->getInterfaceCount());

        for(size_t i = 0; i < classDesc->getInterfaceCount(); ++i)
        {
            const IInterfaceInfo& api = classDesc->getInterface(i);
            std::cout << std::format("Methods Count [{}]\n", api.getMethodsCount());

            for(size_t u = 0; u < api.getMethodsCount(); ++u)
            {
                const IMethodInfo& method = api.getMethod(u);
                std::cout << std::format(" * method [{}]\n", method.getName());
            }
        }

        auto* m = classDesc->findMethod("add1");

        DispatchArguments args;
        args.emplace_back(makeValueCopy(10.f));
        args.emplace_back(makeValueCopy(20.f));

        Ptr<> service = classDesc->getConstructor()->invokeToPtr(nullptr, {});
        // auto service = rtti::createInstance<MyService>();
        auto rr = m->invoke(service.get(), std::move(args));
    }

}  // namespace my::test

void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    return ::malloc(size);
}

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    return ::malloc(size);
}
#endif