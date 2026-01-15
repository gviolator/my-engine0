// #my_engine_source_file
// #include "debugger_host_runner.h"
#include "dap_utils.h"
#include "my/async/task_collection.h"
#include "my/async/work_queue.h"
#include "my/debug/dap/dap.h"
#include "my/debug/dap/debugger_host.h"
#include "my/debug/dap/stack_trace_provider.h"
#include "my/diag/assert.h"
#include "my/network/network.h"
#include "my/rtti/rtti_impl.h"
#include "my/serialization/runtime_value_builder.h"
#include "my/utils/string_utils.h"

using namespace my::async;
using namespace my::network;

namespace my::dap {

namespace {

/**
 */
class DebuggerHostRunner final : public IRefCounted,
                                 public IAsyncDisposable
{
    MY_REFCOUNTED_CLASS(my::dap::DebuggerHostRunner, IRefCounted, IAsyncDisposable)

public:
    DebuggerHostRunner(std::unique_ptr<IDebuggerHost>&& host, const Ptr<IListener>& listener) :
        m_host(std::move(host)),
        m_listener(listener)
    {
        MY_ASSERT(m_host);
        MY_ASSERT(m_listener);
    }

    void run();

private:
    Task<> disposeAsync() override;

    Task<> runImpl();

    Task<> initAndRunDebugSession(io::AsyncStreamPtr client);

    std::unique_ptr<IDebuggerHost> m_host;
    Ptr<IListener> m_listener;
    Task<> m_task;
    std::atomic<bool> m_isDisposed = false;
};

template <typename T>
struct ResponseOfT
{
    using type = dap::GenericResponseMessage<T>;
};

template <>
struct ResponseOfT<void>
{
    using type = dap::ResponseMessage;
};

template <typename T>
using ResponseOf = typename ResponseOfT<T>::type;

struct PausedExecutionState
{
    ExecutorPtr executor;
    dap::StackTraceProviderPtr stackTraceProvider;

    PausedExecutionState(ExecutorPtr executor_in, dap::StackTraceProviderPtr stackProvider_in) :
        executor(std::move(executor_in)),
        stackTraceProvider(std::move(stackProvider_in))
    {
    }
};

/**
 */
class DebuggerState : public IDebugSession
{
public:
    MY_TYPEID(dap::DebuggerState)
    MY_CLASS_BASE(IDebugSession)

    template <typename R, typename P>
    using RequestHandlerMethod = Result<R> (DebuggerState::*)(P);

    DebuggerState(io::IAsyncStream& stream) :
        m_stream{stream}
    {
    }

    template <typename R, typename P>
    Task<> requestCall(RequestHandlerMethod<R, P> handler, dap::RequestMessage request)
    {
        // static_assert(std::is_base_of_v<dap::ResponseMessage, R>);
        using ParamType = std::decay_t<P>;

        MY_DEBUG_ASSERT(m_debuggee);
        if (auto executor = m_debuggee->getExecutor())
        {
            co_await std::move(executor);
        }

        Result<R> result;

        if constexpr (std::is_same_v<ParamType, RuntimeValuePtr>)
        {
            result = (this->*handler)(std::move(request.arguments));
        }
        else
        {
            ParamType param;
            if (Result<> applyResult = runtimeValueApply(param, request.arguments); !applyResult)
            {
                // TODO: handle cast failure
                // co_yield result.getError();
            }
            result = (this->*handler)(param);
        }

        if (!result)
        {
            dap::ResponseMessage errorResponse{nextSeqId(), std::move(request)};
            errorResponse.setError(result.getError()->getMessage());
            co_await sendResponse(m_stream, errorResponse);
            co_return;
        }

        ResponseOf<R> response{nextSeqId(), std::move(request)};
        if constexpr (!std::is_same_v<R, void>)
        {
            response.body = std::move(*result);
        }

        co_await sendResponse(m_stream, response);
    }

    void setDebuggee(IDebuggeeControl& debuggee)
    {
        MY_DEBUG_ASSERT(!m_debuggee);
        m_debuggee = &debuggee;
    }

    unsigned nextSeqId()
    {
        return ++m_nextSeqId;
    }

    Result<> handleAttach(RuntimeValuePtr configuration)
    {
        return m_debuggee->configureAttach(configuration);
    }

    Result<> handleLaunch(RuntimeValuePtr configuration)
    {
        return m_debuggee->configureLaunch(configuration);
    }

    Result<dap::SetBreakpointsResponseBody> handleSetBreakpoints(const dap::SetBreakpointsArguments& args)
    {
        Result<std::vector<dap::Breakpoint>> breakpoints = m_debuggee->setBreakpoints(args);
        CheckResult(breakpoints);

        return dap::SetBreakpointsResponseBody{
            .breakpoints = std::move(*breakpoints)};
    }

    Result<dap::SetFunctionBreakpointsResponseBody> handleSetFunctionBreakpoints(const dap::SetFunctionBreakpointsArguments& args)
    {
        Result<std::vector<dap::Breakpoint>> breakpoints = m_debuggee->setFunctionBreakpoints(args);
        CheckResult(breakpoints);

        return dap::SetFunctionBreakpointsResponseBody{
            .breakpoints = std::move(*breakpoints)};
    }

    Result<> handleConfigurationDone(const RuntimeValuePtr)
    {
        return m_debuggee->configurationDone();
    }

    Result<dap::ThreadsResponseBody> handleThreads(const RuntimeValuePtr)
    {
        Result<std::vector<dap::Thread>> threads = m_debuggee->getThreads();
        CheckResult(threads);

        return dap::ThreadsResponseBody{
            .threads = std::move(*threads)};
    }

    //Result


    ContinueExecutionMode Pause(dap::StoppedEventBody eventData, StackTraceProviderPtr stackProvider) override
    {
        MY_DEBUG_ASSERT(stackProvider);

        WorkQueuePtr executor = createWorkQueue();

        scope_on_leave
        {
            const std::lock_guard lock{m_pausedStateMutex};
            m_pausedState.reset();
        };

        {
            const std::lock_guard lock{m_pausedStateMutex};
            MY_DEBUG_ASSERT(!m_pausedState);
            m_pausedState.emplace(executor, std::move(stackProvider));
        }

        {
            dap::GenericEventMessage<dap::StoppedEventBody> event{nextSeqId(), kEvent_Stopped};
            event.body = std::move(eventData);
            sendEvent(m_stream, event).detach();
        }

        executor->poll(std::nullopt);

        // TODO: make ret value
        return ContinueExecutionMode::Continue;
    }

#if 0
	CLASS_METHOD(DebugSessionImpl, HandleSetFunctionBreakpoints, RequestHandlerAttribute{"setFunctionBreakpoints"}),
			CLASS_METHOD(DebugSessionImpl, HandleScopes, RequestHandlerAttribute{"scopes"}, UseControllerSchedulerAttribute{}),
			CLASS_METHOD(DebugSessionImpl, HandleStackTrace, RequestHandlerAttribute{"stackTrace"}, UseControllerSchedulerAttribute{}),
			CLASS_METHOD(DebugSessionImpl, HandleVariables, RequestHandlerAttribute{"variables"}, UseControllerSchedulerAttribute{}),
			CLASS_METHOD(DebugSessionImpl, HandleEvaluate, RequestHandlerAttribute{"evaluate"}, UseControllerSchedulerAttribute{}),
			CLASS_METHOD(DebugSessionImpl, HandlePause, RequestHandlerAttribute{"pause"}),
			CLASS_METHOD(DebugSessionImpl, HandleDebugStep, RequestHandlerAttribute{"continue"}),
			CLASS_METHOD(DebugSessionImpl, HandleDebugStep, RequestHandlerAttribute{"next"}),
			CLASS_METHOD(DebugSessionImpl, HandleDebugStep, RequestHandlerAttribute{"stepIn"}),
			CLASS_METHOD(DebugSessionImpl, HandleDebugStep, RequestHandlerAttribute{"stepOut"})
#endif
private:
    IDebuggeeControl* m_debuggee = nullptr;
    io::IAsyncStream& m_stream;
    std::atomic<unsigned> m_nextSeqId{1};
    std::optional<PausedExecutionState> m_pausedState;
    std::mutex m_pausedStateMutex;
};

using DebuggeeRequestCall = Task<> (*)(DebuggerState&, dap::RequestMessage&& request);

template <auto F>
inline DebuggeeRequestCall debuggeeRequest()
{
    return [](DebuggerState& session, dap::RequestMessage&& request) -> Task<>
    {
        return session.requestCall(F, std::move(request));
    };
}

Task<> DebuggerHostRunner::disposeAsync()
{
    co_return;
}

void DebuggerHostRunner::run()
{
    m_task = runImpl();
}

Task<> DebuggerHostRunner::runImpl()
{
    TaskCollection sessionTasks;

    while (!m_isDisposed)
    {
        io::AsyncStreamPtr client = co_await m_listener->accept();
        if (!client)
        {
            break;
        }

        sessionTasks.push(initAndRunDebugSession(std::move(client)));

        /*auto [buffer, content] = co_await readHttpContent(*client);
        std::cout << std::string {reinterpret_cast<const char*>(content.data()), content.size()} << std::endl;*/
    }
}

// class

Task<> DebuggerHostRunner::initAndRunDebugSession(io::AsyncStreamPtr commandStream)
{
    DebuggeeControlPtr debuggee;
    DebuggerState session{*commandStream};

    // const auto reqCall = []<auto F>() -> DebuggeeRequestCall
    //{
    //     return [](DebugSessionState& sess, dap::RequestMessage&& request) -> Task<>
    //     {
    //         return sess.debugeeCall(F, std::move(request));
    //     };
    // };

    const std::map<std::string, DebuggeeRequestCall, strings::CiStringComparer<>> requestHandlers = {
        {kCommand_ConfigurationDone, debuggeeRequest<&DebuggerState::handleConfigurationDone>()},
        {   kCommand_SetBreakpoints,    debuggeeRequest<&DebuggerState::handleSetBreakpoints>()}
    };

    TaskCollection tasks;

    while (true)
    {
        dap::RawPacket packet = co_await dap::readHttpPacket(*commandStream);
        // handle runner specific prior requests DAP requests here (if needed or remove comment).

        Result<dap::RequestMessage> request = parseDapRequest(packet);
        if (!request)
        {
            // TODO: handle bad packet
        }
        // MY_ASSERT(request);

        // std::string command = *runtimeValueCast<std::string>(request->getValue("command"));
        //  if (request != "initialize")
        //  {
        //      // unexpected command
        //      co_return;
        //  }

        if (strings::icaseEqual(request->command, kCommand_Initialize))
        {
            InitializeRequestArguments initializeArgs;
            if (Result<> result = runtimeValueApply(initializeArgs, request->arguments); !result)
            {
                co_yield result.getError();
            }
            else
            {
                MY_DEBUG_ASSERT(!debuggee);

                GenericResponseMessage<Capabilities> response{session.nextSeqId(), std::move(*request)};
                std::tie(response.body, debuggee) = co_await m_host->startDebugSession("_", initializeArgs, session);
                session.setDebuggee(*debuggee);

                co_await sendResponse(*commandStream, response);

                // co_await sendEvent(*commandStream, InitializedEvent{session.nextSeqId()});
            }
        }
        // else
        //{
        //     auto iter = requestHandlers.find(request->command);
        //     if (iter == requestHandlers.end())
        //     {
        //         // TODO: unsupported DAP request
        //     }

        //    const DebuggeeRequestCall handler = iter->second;
        //    Task<> task = handler(session, std::move(*request));
        //    tasks.push(std::move(task));
        //}

        else if (strings::icaseEqual(request->command, kCommand_Attach))
        {
            // tasks.do
            [[maybe_unused]] Result<> attachRes = co_await session.requestCall(&DebuggerState::handleAttach, std::move(*request)).doTry();

            tasks.push([](auto& stream, auto& session) -> Task<>
            {
                co_await std::chrono::seconds(1);

                mylog("SEND INITIALIZED");

                co_await sendEvent(stream, InitializedEvent{session.nextSeqId()});
            }(*commandStream, session));

            //// tasks.push(session.debugeeCall(&IDebuggeeControl::configureAttach, std::move(*request)));
            // GenericResponseMessage<> response{session.nextSeqId(), std::move(*request)};
            // co_await sendGenericResponse(*commandStream, response);

            //

            //
        }
        else if (strings::icaseEqual(request->command, "source"))
        {
            dap::ResponseMessage err;
            err.setError("Invalid sorce ref");
            co_await sendResponse(*commandStream, err);
        }

        else if (auto iter = requestHandlers.find(request->command); iter != requestHandlers.end())
        {
            const DebuggeeRequestCall handler = iter->second;
            Task<> task = handler(session, std::move(*request));
            tasks.push(std::move(task));
        }
        else
        {  // unknown call
            dap::ResponseMessage response{session.nextSeqId(), std::move(*request)};

            co_await sendResponse(*commandStream, response);
        }

        // else if (strings::icaseEqual(request->command, kCommand_ConfigurationDone))
        // {
        //     tasks.push(session.debugeeCall(&IDebuggeeControl::configurationDone, std::move(*request)));
        // }

        // m_host->startDebugSession("default", initializeArgs,
    }

    // std::cout << "Initialize session !\n";
}

}  // namespace
//
// Task<> DebuggerHostRunner::disposeAsync()
//{
//    co_return;
//}

// void DebuggerHostRunner::run(Address&& listenAddress, std::unique_ptr<IDebuggerHost>&& host)
//{
//     m_runTask = [](DebuggerHostRunner& self, Address address, std::unique_ptr<IDebuggerHost> host) -> Task<>
//     {
//         co_return;
//     }(*this, std::move(listenAddress), std::move(host));
// }

Task<Ptr<IAsyncDisposable>> runDebuggerHost(network::Address listenAddress, std::unique_ptr<IDebuggerHost> debuggerHost)
{
    Ptr<network::IListener> listener = co_await network::listen(std::move(listenAddress));

    Ptr<DebuggerHostRunner> runner = rtti::createInstance<DebuggerHostRunner>(std::move(debuggerHost), listener);
    runner->run();

    co_return runner;

    // Task<> processing = [](Ptr<network::IListener> listener) -> Task<>
    //{
    //     co_return;
    // }(std::move(listener));

    // co_await

    //    auto runner = rtti::createInstance<DebuggerHostRunner>();
    // runner->run(std::move(listenAddress), std::move(debuggerHost));

    // co_return nullptr;

    // return runner;
}

}  // namespace my::dap
