// #my_engine_source_file
#pragma once

#include "disposable_runtime_object.h"
#include "my/io/async_stream.h"
#include "my/network/network.h"
#include "my/rtti/rtti_impl.h"
#include "my/runtime/disposable.h"
#include "runtime/uv_handle.h"

namespace my::network {

class StreamSocket final : public io::IAsyncStream,
                           public IEndPoint,
                           public IDisposable,
                           public DisposableRuntimeObject
{
    MY_REFCOUNTED_CLASS(my::network::StreamSocket, IDisposable, io::IAsyncStream, IEndPoint)

    StreamSocket(const StreamSocket&) = delete;
    StreamSocket& operator=(const StreamSocket&) = delete;

public:
    StreamSocket(UvHandle<uv_stream_t>&&);
    ~StreamSocket();

    void dispose() override;
    size_t getPosition() const override;
    size_t setPosition(io::OffsetOrigin origin, int64_t offset) override;
    void flush() override;
    bool canSeek() const override;
    bool canRead() const override;
    bool canWrite() const override;

    async::Task<Buffer> read() override;
    async::Task<> write(ReadOnlyBuffer buffer) override;

    Address getLocalAddress() const override;
    Address getRemoteAddress() const override;

protected:
    const UvHandle<uv_stream_t>& getUvStream();

private:
    Result<> readStart();
    void notifyReadAwaiter();
    void closeStream(bool fromDestructor);

    UvHandle<uv_stream_t> m_stream;
    async::TaskSource<> m_readTaskSource = nullptr;

    Buffer m_readBuffer;
    Buffer m_pendingBuffer;
};

}  // namespace my::network
