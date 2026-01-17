// #my_engine_source_file
#include "my/runtime/internal/kernel_runtime.h"
#include "runtime/uv_utils.h"
#include "stream_socket.h"

using namespace my::async;

namespace my::network {

constexpr size_t MaxReadBufferSize = 1600;

StreamSocket::StreamSocket(UvHandle<uv_stream_t>&& handle) :
    m_stream(std::move(handle))

{
    MY_DEBUG_FATAL(m_stream);
    m_stream.setData(this);
}

StreamSocket::~StreamSocket()
{
    closeStream(true);
}

void StreamSocket::closeStream(bool fromDestructor)
{
    doDispose(fromDestructor, *this, [](IRefCounted& refCountedSelf) noexcept
    {
        StreamSocket& self = refCountedSelf.as<StreamSocket&>();
        if (self.m_stream && uv_is_active(self.m_stream) != 0)
        {
            [[maybe_unused]] const int readStopResult = uv_read_stop(self.m_stream);
        }

        self.m_stream.reset();
        self.notifyReadAwaiter();
    });
}

void StreamSocket::dispose()
{
    closeStream(false);
}

size_t StreamSocket::getPosition() const
{
    MY_DEBUG_ASSERT("StreamSocket::getPosition() not supported");
    return 0;
}

size_t StreamSocket::setPosition(io::OffsetOrigin, int64_t)
{
    MY_DEBUG_ASSERT("StreamSocket::setPosition() not supported");
    return 0;
}

void StreamSocket::flush()
{
}

bool StreamSocket::canSeek() const
{
    return false;
}

bool StreamSocket::canRead() const
{
    return true;
}

bool StreamSocket::canWrite() const
{
    return true;
}

Result<> StreamSocket::readStart()
{
    MY_DEBUG_ASSERT(m_stream);

    if (uv_is_active(m_stream) != 0)
    {
        return kResultSuccess;
    }

    const auto allocateCallback = [](uv_handle_t* handle, size_t requestSize, uv_buf_t* outBuffer) noexcept
    {
        MY_DEBUG_FATAL(handle && handle->data);
        auto& self = *static_cast<StreamSocket*>(handle->data);
        const size_t readSize = std::min(MaxReadBufferSize, requestSize);
        self.m_readBuffer.resize(readSize);

        outBuffer->base = reinterpret_cast<char*>(self.m_readBuffer.data());
        outBuffer->len = static_cast<ULONG>(self.m_readBuffer.size());
    };

    const auto readCallback = [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* inboundBuffer) noexcept
    {
        MY_DEBUG_FATAL(stream && stream->data);
        if (!stream->data)
        {
            return;
        }

        auto& self = *static_cast<StreamSocket*>(stream->data);
        scope_on_leave
        {
            self.notifyReadAwaiter();
        };

        if (nread < 0)
        {
            if (const int reason = static_cast<int>(nread); reason != UV__EOF)
            {
                // [[maybe_unused]] const char* message = uvErrorMessage(reason); //uv_err_name(static_cast<int>(nread));
                mylog_warn("UV Stream stream read error: ({})", getUVErrorMessage(reason));
            }

            self.m_stream.reset();
            return;
        }

        const size_t readCount = static_cast<size_t>(nread);

        MY_DEBUG_ASSERT(self.m_readBuffer && self.m_readBuffer.data() == reinterpret_cast<std::byte*>(inboundBuffer->base));
        MY_DEBUG_ASSERT(readCount <= self.m_readBuffer.size());

        if (!self.m_pendingBuffer)
        {
            self.m_pendingBuffer = std::move(self.m_readBuffer);
            self.m_pendingBuffer.resize(readCount);
        }
        else
        {
            const size_t offset = self.m_pendingBuffer.size();
            self.m_pendingBuffer.resize(offset + readCount);
            memcpy(self.m_pendingBuffer.data() + offset, self.m_readBuffer.data(), readCount);
        }
    };

    if (const auto code = uv_read_start(m_stream, allocateCallback, readCallback); code != 0)
    {
        return MakeError(getUVErrorMessage(code));
    }

    return kResultSuccess;
}

void StreamSocket::notifyReadAwaiter()
{
    MY_DEBUG_ASSERT(getKernelRuntime().isRuntimeThread());
    if (m_readTaskSource)
    {
        std::exchange(m_readTaskSource, nullptr).resolve();
    }
}

async::Task<Buffer> StreamSocket::read()
{
    if (isDisposed())
    {
        co_return MakeError("Object disposed");
    }

    this->addRef();
    scope_on_leave
    {
        this->releaseRef();
    };

    ASYNC_SWITCH_EXECUTOR(getKernelRuntime().getRuntimeExecutor());

    if (!m_pendingBuffer.empty())
    {
        co_return std::exchange(m_pendingBuffer, Buffer{});
    }

    if (!m_stream)
    {
        co_return MakeError("Object is not readable");
    }

    if (m_readTaskSource)
    {
        MY_DEBUG_FAILURE("read operation already in progress");
        co_return MakeError("read operation already in progress");
    }

    if (uv_is_active(m_stream) == 0)
    {
        Result<> result = readStart();
        if (!result)
        {
            co_return result.getError();
        }
    }

    if (m_pendingBuffer.empty())
    {
        m_readTaskSource = {};
        co_await m_readTaskSource.getTask();
    }

    co_return std::exchange(m_pendingBuffer, nullptr);
}

async::Task<> StreamSocket::write(ReadOnlyBuffer buffer)
{
    if (isDisposed())
    {
        co_yield MakeError("Object disposed");
    }

    this->addRef();
    scope_on_leave
    {
        this->releaseRef();
    };

    IKernelRuntime& runtime = getKernelRuntime();
    ASYNC_SWITCH_EXECUTOR(runtime.getRuntimeExecutor());

    if (!m_stream || uv_is_writable(m_stream) == 0)
    {
        co_yield MakeError("Object is not writable");
    }

    std::byte* const mutablePtr = const_cast<std::byte*>(buffer.data());

    // buffer will keep its reference during call and will be automatically released after buffer leave its scope.
    uv_buf_t uvBuffer;
    uvBuffer.base = reinterpret_cast<char*>(mutablePtr);
    uvBuffer.len = static_cast<ULONG>(buffer.size());

    TaskSource<> taskSource;
    uv_write_t request;
    request.data = &taskSource;

    const auto result = uv_write(&request, m_stream, &uvBuffer, 1, [](uv_write_t* request, int status)
    {
        MY_DEBUG_FATAL(request && request->data);

        auto& taskSource = *reinterpret_cast<TaskSource<>*>(request->data);
        if (status != 0)
        {
            const std::string_view message = getUVErrorMessage(status);
            taskSource.reject(MakeError(message));
        }
        else
        {
            taskSource.resolve();
        }
    });

    if (result != 0)
    {
        co_return;
    }

    co_await taskSource.getTask();
}

Address StreamSocket::getLocalAddress() const
{
    return {};
}

Address StreamSocket::getRemoteAddress() const
{
    return {};
}

const UvHandle<uv_stream_t>& StreamSocket::getUvStream()
{
    return (m_stream);
}

}  // namespace my::network