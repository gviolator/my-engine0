// #my_engine_source_file
#include "listener.h"
#include "my/diag/logging.h"
#include "my/threading/lock_guard.h"
#include "runtime/uv_utils.h"
#include "stream_socket.h"

using namespace my::async;

namespace my::network {

Listener::Listener(Address&& address, UvHandle<uv_stream_t>&& handle, ListenOptions) :
    m_address(std::move(address)),
    m_uvServer(std::move(handle)),
    m_runtimeReg{*this}
{
    MY_DEBUG_FATAL(m_uvServer);
    uv_handle_set_data(m_uvServer, this);
}

Listener::~Listener()
{
    closeListener(true);
}

void Listener::dispose()
{
    closeListener(false);
}

void Listener::closeListener(bool fromDestructor)
{
    doDispose(fromDestructor, *this, [](IRefCounted& refCountedSelf) noexcept
    {
        Listener& self = refCountedSelf.as<Listener&>();

        self.m_uvServer.reset();
        self.doAccept(nullptr);
    });
}

Result<> Listener::startListen(unsigned backlog)
{
    const auto connectionCallback = [](uv_stream_t* listenerHandle, int status) noexcept
    {
        MY_DEBUG_ASSERT(listenerHandle && listenerHandle->data);
        if (!listenerHandle || !listenerHandle->data)
        {
            mylog_error("connection callback called with invalid data");
            return;
        }

        Listener& self = *static_cast<Listener*>(listenerHandle->data);
        const std::lock_guard lock(self.m_mutex);
        if (status != 0)
        {
            self.doAccept(nullptr);
            return;
        }

        // Actual uv_accept should not be delayed and stream must be created here:
        // 1. can call uv_accept and create actual Stream only when called Listener::accept;
        // 2. if callee will not call Listener::accept for each inbound connection there is possibility that UV will not close sockets for this inbound connections
        //      and because connection actually established and socket remaining alive remote client will hang forever on read operation
        //      i.e. Client::connect finished with success and subsequent ::read() will never be finished.

        UvHandle<uv_stream_t> clientHandle = self.initClientHandle();
        if (const auto acceptResult = uv_accept(listenerHandle, clientHandle); acceptResult != 0)
        {
        }
        else
        {
            Ptr<StreamSocket> connection = rtti::createInstance<StreamSocket>(std::move(clientHandle));
            self.doAccept(std::move(connection));
        }
    };

    if (const int error = uv_listen(m_uvServer, static_cast<int>(backlog), connectionCallback); error != 0)
    {
        return MakeError(getUVErrorMessage(error));
    }

    return ResultSuccess;
}

Address Listener::getLocalAddress() const
{
    return m_address;
}

Address Listener::getRemoteAddress() const
{
    return Address{};
}

Task<io::AsyncStreamPtr> Listener::accept()
{
    const std::lock_guard lock(m_mutex);

    if (!m_uvServer)
    {
        return Task<io::AsyncStreamPtr>::makeResolved(nullptr);
    }

    MY_DEBUG_ASSERT(!m_acceptTaskSource, "Accept already processed");
    if (m_acceptTaskSource)
    {
        return Task<io::AsyncStreamPtr>::makeRejected(MakeError("Accept already processed"));
    }

    if (!m_delayedConnections.empty())
    {
        io::AsyncStreamPtr stream = std::move(m_delayedConnections.front());
        m_delayedConnections.pop_front();
        return Task<io::AsyncStreamPtr>::makeResolved(std::move(stream));
    }

    m_acceptTaskSource = {};  // initialize task source
    return m_acceptTaskSource.getTask();
}

void Listener::doAccept(io::AsyncStreamPtr stream)
{
    // Expected tha m_mutex are locked by caller.
    if (m_acceptTaskSource)
    {
        std::exchange(m_acceptTaskSource, nullptr).resolve(std::move(stream));
    }
    else
    {
        m_delayedConnections.emplace_back(std::move(stream));
    }
}

}  // namespace my::network
