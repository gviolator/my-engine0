// #my_engine_source_file
#pragma once
#include "my/io/stream.h"
#include "my/async/task_base.h"
#include "my/memory/buffer.h"


namespace my::io
{
    struct MY_ABSTRACT_TYPE IAsyncStream : IStreamBase
    {
        MY_INTERFACE(my::io::IAsyncStream, IStreamBase)

        virtual async::Task<Buffer> read() = 0;

        virtual async::Task<> write(ReadOnlyBuffer buffer) = 0;
    };

    using AsyncStreamPtr = my::Ptr<IAsyncStream>;

}