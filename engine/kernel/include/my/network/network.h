// #my_engine_source_file
#pragma once

#include "my/async/task_base.h"
#include "my/io/async_stream.h"
#include "my/kernel/kernel_config.h"
#include "my/network/address.h"
#include "my/rtti/rtti_object.h"
#include "my/runtime/disposable.h"
#include "my/utils/cancellation.h"

// #include <tuple>

namespace my::network {

/**
 */
enum class SocketProtocol
{
    Unknown,
    Stream,
    Datagram
};

struct IEndPoint : virtual IRefCounted
{
    MY_INTERFACE(my::network::IEndPoint, IRefCounted)

    virtual Address getLocalAddress() const = 0;
    virtual Address getRemoteAddress() const = 0;
};

/**
 */
struct MY_ABSTRACT_TYPE IStreamSocketControl
{
    MY_TYPEID(my::network::IStreamSocketControl)

    virtual size_t setInboundBufferSize(size_t size) = 0;
};

/**
 */
struct MY_ABSTRACT_TYPE IListener : IEndPoint, IDisposable
{
    MY_INTERFACE(my::network::IListener, IEndPoint, IDisposable)

    virtual async::Task<io::AsyncStreamPtr> accept() = 0;
};

struct ListenOptions
{
    unsigned backlog = 0;
};

MY_KERNEL_EXPORT async::Task<Ptr<IListener>> listen(Address address, ListenOptions = {});

MY_KERNEL_EXPORT async::Task<io::AsyncStreamPtr> connect(Address address, Address bindAddress = {}, Expiration expiration = Expiration::never());

}  // namespace my::network
