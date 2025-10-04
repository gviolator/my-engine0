// #my_engine_source_file
#include "address_impl.h"
#include "listener.h"
#include "tcp.h"
#include "my/network/network.h"
#include "runtime/kernel_runtime_impl.h"
#include "runtime/uv_handle.h"
#include "runtime/uv_utils.h"
#include "stream_socket.h"


using namespace my::async;

namespace my::network {


Task<Ptr<IListener>> listen(Address address, ListenOptions options)
{
    MY_DEBUG_ASSERT(address);
    if (!address)
    {
        co_return nullptr;
    }

    KernelRuntimeImpl& runtime = getKernelRuntimeImpl();
    ASYNC_SWITCH_EXECUTOR(runtime.getRuntimeExecutor());

    Ptr<Listener> listener;
    unsigned backlog = options.backlog;
    for (IAddress* const bindAddress : address)
    {
        MY_DEBUG_ASSERT(bindAddress);
        if (!bindAddress)
        {
            continue;
        }

        if (InetAddress* const addr = bindAddress->as<InetAddress*>())
        {
            UvHandle<uv_tcp_t> tcp;
            UV_VERIFY(uv_tcp_init(runtime.uv(), tcp));
            // When address in use UV_EADDRINUSE will occur only in listen operation (inside startListen).
            if (const int bindResult = uv_tcp_bind(tcp, addr->getSockAddr(), 0); bindResult != 0)
            {
                
            }
            else
            {
                if (backlog == 0)
                {
                    backlog = SOMAXCONN;
                }
                listener = createTcpListener(Address{addr}, std::move(tcp), options);
                break;
            }
        }
        else
        {
        }
    }

    if (listener)
    {
        if (auto res = listener->startListen(backlog); !res)
        {
            co_return res.getError();
        }
    }
    else
    {
    }

    co_return listener;
}

MY_KERNEL_EXPORT Task<io::AsyncStreamPtr> connect(Address address, Address bindAddress, [[maybe_unused]] Expiration expiration)
{
    MY_DEBUG_ASSERT(address);
    if (!address)
    {
        co_return nullptr;
    }

    KernelRuntimeImpl& runtime = getKernelRuntimeImpl();
    ASYNC_SWITCH_EXECUTOR(runtime.getRuntimeExecutor());

    Ptr<StreamSocket> socket;
    for (IAddress* const remoteAddress : address)
    {
        MY_DEBUG_ASSERT(remoteAddress);
        if (!remoteAddress)
        {
            continue;
        }

        if (const InetAddress* const addr = remoteAddress->as<const InetAddress*>())
        {
            UvHandle<uv_tcp_t> tcp;
            UV_VERIFY(uv_tcp_init(runtime.uv(), tcp));

            if (bindAddress)
            {
                const Result<bool> bindResult = tcpBind(tcp, bindAddress);
                if (bindResult.isError())
                {
                    co_return bindResult.getError();
                }

                const bool bindOk = *bindResult;
                if (!bindOk)
                {
                    co_return MakeError("Can not bind socket to specified address");
                }
            }

            const Result<bool> connectResult = co_await tcpConnect(tcp, addr->getSockAddr()).doTry();
            if (connectResult.isError())
            {
                co_return connectResult.getError();
            }

            const bool connectOk = *connectResult;
            if (connectOk)
            {
                socket = rtti::createInstance<StreamSocket>(std::move(tcp));
                break;
            }
        }
    }

    co_return socket;
}

}  // namespace my::network
