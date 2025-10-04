// #my_engine_source_file
#pragma once
#include "my/async/task.h"
#include "my/io/stream.h"
#include "my/memory/buffer.h"

namespace my::io {

/**
 */
struct MY_ABSTRACT_TYPE IAsyncStream : IStreamBase
{
    MY_INTERFACE(my::io::IAsyncStream, IStreamBase)

    virtual async::Task<Buffer> read() = 0;

    // TODO: ReadOnlyBuffer should be replaced with BufferView ???
    virtual async::Task<> write(ReadOnlyBuffer buffer) = 0;
};

using AsyncStreamPtr = my::Ptr<IAsyncStream>;

}  // namespace my::io
