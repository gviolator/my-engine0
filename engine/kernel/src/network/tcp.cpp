// #my_engine_source_file

#include "address_impl.h"
#include "my/rtti/rtti_impl.h"
#include "runtime/kernel_runtime_impl.h"
#include "runtime/uv_utils.h"
#include "tcp.h"
// #include "stream_socket.h"

using namespace my::async;

namespace my::network {

class TcpListener final : public Listener
{
    MY_REFCOUNTED_CLASS(my::network::TcpListener, Listener)

public:
    TcpListener(Address&& address, UvHandle<uv_stream_t>&& handle, ListenOptions options) :
        Listener(std::move(address), std::move(handle), options)
    {
    }

private:
    UvHandle<uv_stream_t> initClientHandle() override
    {
        UvHandle<uv_tcp_t> tcp;
        UV_VERIFY(uv_tcp_init(getKernelRuntimeImpl().uv(), tcp));
        return tcp;
    }
};

Task<bool> tcpConnect(uv_tcp_t* handle, const sockaddr* address)
{
    MY_DEBUG_ASSERT(handle);
    MY_DEBUG_ASSERT(address);

    TaskSource<bool> taskSource;
    uv_connect_t request;
    request.data = &taskSource;

    const int tcpConnectResult = uv_tcp_connect(&request, handle, address, [](uv_connect_t* connect, int status)
    {
        MY_FATAL(connect && connect->data);

        TaskSource<bool>& taskSource = *static_cast<TaskSource<bool>*>(connect->data);
        if (status == 0)
        {
            taskSource.resolve(true);
        }
        else if (status == UV_ECONNREFUSED)
        {
            taskSource.resolve(false);
        }
        else
        {
            const std::string_view error = getUVErrorMessage(status);
            taskSource.reject(MakeError(error));
        }
    });

    UV_VERIFY(tcpConnectResult);

    const bool connectedOk = co_await taskSource.getTask();
    co_return connectedOk;
}

Result<bool> tcpBind(uv_tcp_t* handle, const Address& address)
{
    for (const IAddress* const bindAddress : address)
    {
        MY_DEBUG_ASSERT(bindAddress);
        if (!bindAddress)
        {
            continue;
        }

        const InetAddress* const inetAddress = bindAddress->as<const InetAddress*>();
        MY_DEBUG_ASSERT(inetAddress, "Address expected to be Inet Address");
        if (!inetAddress)
        {
            return MakeError("Address expected to be Inet Address");
        }

        const int bindResult = uv_tcp_bind(handle, inetAddress->getSockAddr(), 0);
        if (bindResult == 0)
        {
            return true;
        }

        if (bindResult == UV_EADDRINUSE)
        {
            continue;
        }

        const std::string_view message = getUVErrorMessage(bindResult);
        return MakeError("tcp_bind failure:({})", message);
    }

    return false;
}

Ptr<Listener> createTcpListener(Address&& address, UvHandle<uv_tcp_t>&& handle, ListenOptions options)
{
    return rtti::createInstance<TcpListener>(std::move(address), std::move(handle), options);
}

}  // namespace my::network
