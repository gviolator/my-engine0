// #my_engine_source_file


#include "my/io/memory_stream.h"

#include "my/diag/assert.h"
#include "my/memory/buffer.h"
#include "my/rtti/rtti_impl.h"

namespace my::io
{
    /**
     */
    class MemoryStream final : public IMemoryStream
    {
        MY_REFCOUNTED_CLASS(my::io::MemoryStream, IMemoryStream)
    public:
        MemoryStream() = default;
        MemoryStream(Buffer buffer);

        size_t getPosition() const override;

        size_t setPosition(OffsetOrigin origin, int64_t offset) override;

        Result<size_t> read(std::byte* buffer, size_t count) override;

        Result<size_t> write(const std::byte*, size_t count) override;

        void flush() override;

        bool canSeek() const override;
        bool canRead() const override;
        bool canWrite() const override;

        std::span<const std::byte> getBufferAsSpan(size_t offset, std::optional<size_t> size) const override;

    private:
        Buffer m_buffer;
        size_t m_pos = 0;
    };

    class ReadOnlyMemoryStream final : public IMemoryStream
    {
        MY_REFCOUNTED_CLASS(my::io::ReadOnlyMemoryStream, IMemoryStream)
    public:
        ReadOnlyMemoryStream() = default;
        ReadOnlyMemoryStream(std::span<const std::byte> buffer);

    private:
        Result<size_t> read(std::byte* buffer, size_t count) override;

        Result<size_t> write(const std::byte*, size_t count) override;

        size_t getPosition() const override;

        size_t setPosition(OffsetOrigin origin, int64_t offset) override;

        void flush() override;

        bool canSeek() const override;
        bool canRead() const override;
        bool canWrite() const override;

        std::span<const std::byte> getBufferAsSpan(size_t offset, std::optional<size_t> size) const override;

    private:
        std::span<const std::byte> m_buffer;
        size_t m_pos = 0;
    };


    MemoryStream::MemoryStream(Buffer buffer) :
        m_buffer(std::move(buffer))
    {
    }

    Result<size_t> MemoryStream::read(std::byte* buffer, size_t count)
    {
        MY_FATAL(m_pos <= m_buffer.size());

        const size_t availableSize = m_buffer.size() - m_pos;
        const size_t actualReadCount = std::min(availableSize, count);

        if(actualReadCount == 0)
        {
            return 0;
        }
        memcpy(buffer, m_buffer.data() + m_pos, actualReadCount);
        m_pos += actualReadCount;
        return actualReadCount;
    }

    Result<size_t> MemoryStream::write(const std::byte* buffer, size_t count)
    {
        MY_FATAL(m_pos <= m_buffer.size());
        const size_t availableSize = m_buffer.size() - m_pos;
        if(availableSize < count)
        {
            const auto needBytes = count - availableSize;
            m_buffer.append(needBytes);
        }

        std::byte* const data = m_buffer.data() + m_pos;
        memcpy(data, buffer, count);
        m_pos += count;

        return count;
    }

    size_t MemoryStream::getPosition() const
    {
        return m_pos;
    }

    size_t MemoryStream::setPosition(OffsetOrigin origin, int64_t offset)
    {
        int64_t newPos = offset;  // OffsetOrigin::Begin
        const int64_t currentSize = static_cast<int64_t>(m_buffer.size());

        if(origin == OffsetOrigin::Current)
        {
            newPos = static_cast<int64_t>(m_pos) + offset;
        }
        else if(origin == OffsetOrigin::End)
        {
            newPos = currentSize + offset;
        }
#if MY_DEBUG_ASSERT_ENABLED
        else
        {
            MY_DEBUG_ASSERT(origin == OffsetOrigin::Begin);
        }
#endif

        if(newPos < 0)
        {
            newPos = 0;
        }
        else if(currentSize < newPos)
        {
            newPos = currentSize;
        }

        MY_FATAL(newPos >= 0);
        MY_FATAL(newPos <= currentSize);

        m_pos = static_cast<size_t>(newPos);
        return m_pos;
    }

    void MemoryStream::flush()
    {
    }

    bool MemoryStream::canSeek() const
    {
        return true;
    }

    bool MemoryStream::canRead() const
    {
        return true;
    }

    bool MemoryStream::canWrite() const
    {
        return true;
    }

    std::span<const std::byte> MemoryStream::getBufferAsSpan(size_t offset, std::optional<size_t> size) const
    {
        MY_DEBUG_ASSERT(offset >= 0 && offset <= m_buffer.size(), "Invalid offset");
        MY_DEBUG_ASSERT(!size || (offset + *size <= m_buffer.size()));

        const size_t actualOffset = std::min(offset, m_buffer.size());
        const size_t actualSize = size.value_or(m_buffer.size() - actualOffset);

        return {m_buffer.data() + actualOffset, actualSize};
    }


    ReadOnlyMemoryStream::ReadOnlyMemoryStream(std::span<const std::byte> buffer) :
        m_buffer(buffer)
    {
    }

    Result<size_t> ReadOnlyMemoryStream::read(std::byte* buffer, size_t count)
    {
        MY_FATAL(m_pos <= m_buffer.size());

        const size_t availableSize = m_buffer.size() - m_pos;
        const size_t actualReadCount = std::min(availableSize, count);

        if (actualReadCount == 0)
        {
            return 0;
        }
        memcpy(buffer, m_buffer.data() + m_pos, actualReadCount);
        m_pos += actualReadCount;
        return actualReadCount;
    }

    Result<size_t> ReadOnlyMemoryStream::write([[maybe_unused]] const std::byte* buffer, [[maybe_unused]] size_t count)
    {
        MY_FAILURE("Invalid operation");
        return 0;
    }

    size_t ReadOnlyMemoryStream::getPosition() const
    {
        return m_pos;
    }

    size_t ReadOnlyMemoryStream::setPosition(OffsetOrigin origin, int64_t offset)
    {
        int64_t newPos = offset;
        const int64_t currentSize = static_cast<int64_t>(m_buffer.size());

        if (origin == OffsetOrigin::Current)
        {
            newPos = static_cast<int64_t>(m_pos) + offset;
        }
        else if (origin == OffsetOrigin::End)
        {
            newPos = currentSize + offset;
        }
#if MY_DEBUG_ASSERT_ENABLED
        else
        {
            MY_DEBUG_ASSERT(origin == OffsetOrigin::Begin);
        }
#endif

        if (newPos < 0)
        {
            newPos = 0;
        }
        else if (currentSize < newPos)
        {
            newPos = currentSize;
        }

        MY_FATAL(newPos >= 0);
        MY_FATAL(newPos <= currentSize);

        m_pos = static_cast<size_t>(newPos);
        return m_pos;
    }

    void ReadOnlyMemoryStream::flush()
    {
    }

    bool ReadOnlyMemoryStream::canSeek() const
    {
        return true;
    }

    bool ReadOnlyMemoryStream::canRead() const
    {
        return true;
    }

    bool ReadOnlyMemoryStream::canWrite() const
    {
        return false;
    }

    std::span<const std::byte> ReadOnlyMemoryStream::getBufferAsSpan(size_t offset, std::optional<size_t> size) const
    {
        MY_DEBUG_ASSERT(offset >= 0 && offset <= m_buffer.size(), "Invalid offset");
        MY_DEBUG_ASSERT(!size || (offset + *size <= m_buffer.size()));

        const size_t actualOffset = std::min(offset, m_buffer.size());
        const size_t actualSize = size.value_or(m_buffer.size() - actualOffset);

        return { m_buffer.data() + actualOffset, actualSize };
    }

    MemoryStreamPtr createMemoryStream([[maybe_unused]] AccessModeFlag accessMode, IMemAllocator* allocator)
    {
        return rtti::createInstanceWithAllocator<MemoryStream, IMemoryStream>(allocator);
    }

    MemoryStreamPtr createReadonlyMemoryStream(std::span<const std::byte> buffer, IMemAllocator* allocator)
    {
        return rtti::createInstanceWithAllocator<ReadOnlyMemoryStream, IMemoryStream>(std::move(allocator), std::move(buffer));
    }

    MemoryStreamPtr createMemoryStream(Buffer buffer, [[maybe_unused]] AccessModeFlag accessMode, IMemAllocator* allocator)
    {
        return rtti::createInstanceWithAllocator<MemoryStream, IMemoryStream>(std::move(allocator), std::move(buffer));
    }
}  // namespace my::io