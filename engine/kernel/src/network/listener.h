// #my_engine_source_file
#pragma once
#include "disposable_runtime_object.h"
#include "my/network/network.h"
#include "my/runtime/internal/runtime_object_registry.h"
#include "my/threading/critical_section.h"
#include "runtime/uv_handle.h"

#include <atomic>

namespace my::network {

/**
 */
class MY_ABSTRACT_TYPE Listener : public IListener, public DisposableRuntimeObject
{
    MY_INTERFACE(my::network::Listener, IListener)

    Listener() = delete;
    Listener(const Listener&) = delete;
    Listener& operator=(const Listener&) = delete;

public:
    ~Listener();
    Address getLocalAddress() const final;
    Address getRemoteAddress() const final;
    void dispose() final;
    async::Task<io::AsyncStreamPtr> accept() final;
    Result<> startListen(unsigned backlog);

protected:
    Listener(Address&& address, UvHandle<uv_stream_t>&&, ListenOptions options);

    virtual UvHandle<uv_stream_t> initClientHandle() = 0;

private:
    void doAccept(io::AsyncStreamPtr);
    void closeListener(bool fromDestructor);

    const Address m_address;
    UvHandle<uv_stream_t> m_uvServer;  // stream = tcp, pipe

    async::TaskSource<io::AsyncStreamPtr> m_acceptTaskSource = nullptr;
    std::list<io::AsyncStreamPtr> m_delayedConnections;
    threading::CriticalSection m_mutex;
    RuntimeObjectRegistration m_runtimeReg;
    //std::atomic<bool> m_isClosed = false;
};

Ptr<Listener> createPipeListener(Address&& address, UvHandle<uv_pipe_t>&& handle);

}  // namespace my::network
