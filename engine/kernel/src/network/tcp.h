// #my_engine_source_file
#pragma once

#include "listener.h"
#include "my/async/task.h"
#include "my/network/network.h"
#include "runtime/uv_handle.h"

namespace my::network {

Ptr<Listener> createTcpListener(Address&& address, UvHandle<uv_tcp_t>&& handle, ListenOptions options);

async::Task<bool> tcpConnect(uv_tcp_t* handle, const sockaddr* address);

Result<bool> tcpBind(uv_tcp_t* handle, const Address& address);

}  // namespace my::network
