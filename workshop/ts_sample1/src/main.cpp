#include "my/app/property_container.h"
#include "my/async/async_timer.h"
#include "my/async/executor.h"
#include "my/async/thread_pool_executor.h"
#include "my/async/work_queue.h"
#include "my/io/virtual_file_system.h"
#include "my/memory/buffer.h"
#include "my/rtti/rtti_impl.h"
#include "my/service/internal/service_provider_initialization.h"
#include "my/service/service_provider.h"
#include "my/utils/uid.h"
#include "script_manager.h"

#include "my/app/application.h"

using namespace std::chrono_literals;

namespace my
{
    // inline namespace pooke
    // {
    //     template<typename T>
    //     struct Book
    //     {};
    // }

    // template<>
    // struct Book<int>
    // {};

    class COutput final : public diag::LogSink
    {
    public:
        MY_REFCOUNTED_CLASS(COutput, diag::LogSink)

        void log(const diag::LogMessage& message) override
        {
            std::cout << message.text << std::endl;
        }
    };

    struct EngineState
    {
        std::atomic<bool> completed{false};
        async::ExecutorPtr m_defaultExecutor;

        EngineState()
        {
            async::ITimerManager::setDefaultInstance();
            m_defaultExecutor = async::createThreadPoolExecutor(4);

            async::Executor::setDefault(m_defaultExecutor);

            auto logger = diag::createLogger();
            diag::setDefaultLogger(logger);
            m_logSubscriptions.emplace_back(logger->subscribe(rtti::createInstance<COutput>()));

            mylog_info("Start");

            setDefaultServiceProvider(createServiceProvider());

            // core basic services:
            ServiceProvider& serviceProvider = getServiceProvider();

            auto vfs = io::createVirtualFileSystem();

            vfs->mount("/scripts", io::createNativeFileSystem(R"-(c:\proj\my-engine0\workshop\ts_sample1\content\scripts\out.lua)-")).ignore();

            // {
            //     auto file = vfs->openFile("/scripts/script1.lua", io::AccessMode::Read, io::OpenFileMode::OpenExisting);

            //     auto sz = file->getSize();

            //     io::StreamPtr stream = file->createStream();

            //     Buffer buffer(sz);

            //     stream->read(buffer.data(), buffer.size()).ignore();

            //     std::cout << std::string_view {reinterpret_cast<const char*>(buffer.data()), buffer.size()} << std::endl;
            // }

            serviceProvider.addService(std::move(vfs));
            serviceProvider.addService(createPropertyContainer());
            serviceProvider.addService(std::make_unique<LuaScriptManager>());

            serviceProvider.get<PropertyContainer>().setValue("/scripts/searchPaths", std::vector<std::string>{"/scripts"}).ignore();

            m_appWorkQueue = createWorkQueue();
            m_appWorkQueue->setName("App Work Queue");
            async::Executor::setThisThreadExecutor(m_appWorkQueue);

            const auto waitTaskAndPoll = [&](async::Task<> task) -> Result<>
            {
                while (!task.isReady())
                {
                    m_appWorkQueue->poll();
                }

                return !task.isRejected() ? Result<>{} : task.getError();
            };

            auto& serviceProviderInit = serviceProvider.as<core_detail::IServiceProviderInitialization&>();

            // TODO: check preInit result
            waitTaskAndPoll(serviceProviderInit.preInitServices()).ignore();

            // TODO: check init result
            waitTaskAndPoll(serviceProviderInit.initServices()).ignore();
        }

        ~EngineState()
        {
            diag::setDefaultLogger(nullptr);
        }

        std::vector<diag::LogSubscription> m_logSubscriptions;
        WorkQueuePtr m_appWorkQueue;
    };

    // lua_State* g_Coro0 = nullptr;
    std::string g_CoroId;

    template <typename T>
    int wrapNativePromise(lua_State* l, async::Task<T>& task)
    {
        using namespace my::async;

        CoreTask* const coreTask = getCoreTask(task);
        coreTask->addRef();

        MY_ASSERT(lua_getglobal(l, "setupNativePromise") == LUA_TFUNCTION);
        lua_pcallk(l, 0, 1, 0, 0, nullptr);
        
        lua_getfield(l, -1, "id");
        std::string promiseId = *lua::cast<std::string>(l, -1);
        Buffer b(promiseId.size()+1);
        memcpy(b.data(), promiseId.data(), promiseId.size());
        reinterpret_cast<char*>(b.data())[promiseId.size()] = 0;

        lua_pop(l,1);

        lua_getfield(l, -1, "promise");
        lua_remove(l, -2);

        Executor::Invocation callback{[](void* taskPtr, void* bufferHandle ) noexcept
        {
            Buffer buffer = BufferStorage::bufferFromHandle(reinterpret_cast<BufferHandle>(bufferHandle));
            CoreTask* const coreTask = reinterpret_cast<CoreTask*>(taskPtr);
            Task<T> task = Task<T>::fromCoreTask(CoreTaskOwnership{coreTask});

            MY_DEBUG_ASSERT(task.isReady());


            lua_State* const l = getServiceProvider().get<LuaScriptManager>().getLua();
            const lua::StackGuard sg{l};

            [[maybe_unused]] const int type = lua_getglobal(l, "finalizeNativePromise");
            MY_DEBUG_ASSERT(type == LUA_TFUNCTION);

            const char* const promiseId = reinterpret_cast<const char*>(buffer.data());
            ErrorPtr error = coreTask->getError();
            const bool success = !static_cast<bool>(error);
            lua_pushstring(l, promiseId);
            lua_pushboolean(l, success);

            if (error)
            {
                
            }
            else
            {
                if constexpr (std::is_same_v<T, void>)
                {
                    lua_pushnil(l);
                }
                else
                {
                    T result = task.result();
                    lua::pushRuntimeValue(l, makeValueRef(result)).ignore();
                }
            }

            lua_pcallk(l, 3, 1, 0, 0, nullptr);

        }, coreTask, BufferStorage::takeOut(std::move(b))};

        coreTask->setContinuation(TaskContinuation{std::move(callback), Executor::getCurrent()});

        return 1;
    }

    int myCoro0(lua_State* l) noexcept
    {
        using namespace async;
        Task<std::string> task = []() -> Task<std::string>
        {
            std::cout << "Run c++ async task\n";
            co_await 500ms;
            std::cout << "c++ task completed\n";
            co_return "Some c++ text";
        }();

        return wrapNativePromise(l, task.detach());
    }

    int waitK(lua_State* l) noexcept
    {
        using namespace async;

        unsigned ms = *lua::cast<unsigned>(l, -1);
        Task<> task = [](std::chrono::milliseconds timeout) -> Task<>
        {
            co_await timeout;

        }(std::chrono::milliseconds{ms});

        return wrapNativePromise(l, task.detach());
    }

    //     // lua_pushstring(l, );
    //     const int type = lua_getglobal(l, "setupNativePromise");
    //     lua_pushcclosure(l, [](lua_State* kk) noexcept -> int
    //     {
    //         const lua::StackGuard luaStackGuard{kk};
    //         // g_Coro0 = kk;
    //         g_CoroId = *lua::cast<std::string>(kk, -1);
    //         return 0;
    //     }, 0);

    //     lua_pcallk(l, 1, 1, 0, 0, nullptr);

    //     return 1;
    // }

}  // namespace my

void bar(float&& x) { std::cout << "float " << x << "\n"; }
void bar(int&& x) { std::cout << "int " << x << "\n"; }

// template<typename T>
// void foo(T&& v)
// {
//     bar(v);
// }


int main(int argc, char** argv)
{
    using namespace my;

    auto app = createApplication(nullptr);

    EngineState engine;

    {
        int k = 1;
        bar(static_cast<int&&>(k));
    }

    // foo(1);
    // foo(2.0f);

    {
        lua_State* const l = getServiceProvider().get<LuaScriptManager>().getLua();
        lua_pushcclosure(l, myCoro0, 0);
        lua_setglobal(l, "myCoroCpp");

        lua_pushcclosure(l, waitK, 0);
        lua_setglobal(l, "waitK");

        lua_pushcclosure(l, [](lua_State* l) noexcept -> int
        {
            const std::string str = toString(Uid::generate());
            lua::pushRuntimeValue(l, makeValueRef(str)).ignore();
            return 1;
        }, 0);
        lua_setglobal(l, "generateUid");

        lua_createtable(l, 0, 0);
        lua_setglobal(l, "RefsTable");

        Result<> res = getServiceProvider().get<LuaScriptManager>().executeFileInternal("script1");
        if (!res)
        {
            std::cerr << res.getError()->getMessage() << std::endl;
            return 1;
        }

        // lua_pushcclosure(l, [](lua_State* l) noexcept -> int
        // {

        //     // lua_yieldk([]

        //     // return 0;
        // }, 0);

        // lua_setglobal(l, "waitK");
    }

    {
        lua_State* const l = getServiceProvider().get<LuaScriptManager>().getLua();
        const lua::StackGuard sg{l};

        lua_getglobal(l, "gameInit");
        if (lua_pcallk(l, 0, 0, 0, 0, nullptr) != 0)
        {
            std::string message = *lua::cast<std::string>(l, -1);
            std::cerr << message << std::endl;
        }
    }

    // async::Task<> process = [](EngineState& eng) -> async::Task<>
    // {
    //     scope_on_leave{
    //         // eng.completed = true;
    //         // eng.m_appWorkQueue->notify();
    //     };

    //     co_await 500ms;

    //     lua_State* const l = getServiceProvider().get<LuaScriptManager>().getLua();
    //     // MY_DEBUG_ASSERT(g_Coro0 != nullptr);
    //     MY_DEBUG_ASSERT(!g_CoroId.empty());

    //     const lua::StackGuard sg{l};

    //     const int type = lua_getglobal(l, "finalizeNativePromise");
    //     MY_DEBUG_ASSERT(type == LUA_TFUNCTION);

    //     lua_pushstring(l, g_CoroId.c_str());
    //     lua_pushboolean(l, 1);
    //     lua_pushstring(l, "Task<> result");
    //     lua_pcallk(l, 3, 1, 0, 0, nullptr);
    // }(engine);

    while (!engine.completed.load(std::memory_order_relaxed))
    {
        engine.m_appWorkQueue->poll(0ms);

        {
            lua_State* const l = getServiceProvider().get<LuaScriptManager>().getLua();
            const lua::StackGuard sg{l};

            lua_getglobal(l, "gameMain");
            lua_pushnumber(l, 1.0);
            if (lua_pcallk(l, 1, 0, 0, 0, nullptr) != 0)
            {
                std::string message = *lua::cast<std::string>(l, -1);
                std::cerr << message << std::endl;
            }
        }

        std::this_thread::sleep_for(100ms);
    }

    return 0;
}
