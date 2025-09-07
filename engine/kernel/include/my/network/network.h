// #my_engine_source_file
#pragma once

#include "my/async/task_base.h"
#include "my/io/async_stream.h"
#include "my/kernel/kernel_config.h"
#include "my/rtti/rtti_object.h"
#include "my/utils/cancellation.h"

namespace my::network
{
    struct MY_ABSTRACT_TYPE IServer : IRefCounted
    {
        MY_INTERFACE(IServer, IRefCounted)

        virtual async::Task<io::AsyncStreamPtr> accept() = 0;
    };

    struct MY_ABSTRACT_TYPE ISocketControl
    {
        MY_TYPEID(my::network::ISocketControl)

        virtual size_t setInboundBufferSize(size_t size) = 0;
    };

    using ServerPtr = Ptr<IServer>;

    MY_KERNEL_EXPORT Result<ServerPtr> bind(std::string_view address);

    MY_KERNEL_EXPORT async::Task<io::AsyncStreamPtr> connect(std::string_view address, Expiration expiration);

}  // namespace my::network
