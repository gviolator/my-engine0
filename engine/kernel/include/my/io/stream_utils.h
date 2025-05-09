// #my_engine_source_file

#pragma once
#include <string>
#include <string_view>
#include <type_traits>

#include "my/diag/check.h"
#include "my/io/stream.h"
#include "my/kernel/kernel_config.h"
#include "my/rtti/rtti_impl.h"

namespace my::io_detail
{
    /**
     */
    class StringWriterImpl final : public io::IStream
    {
        MY_REFCOUNTED_CLASS(StringWriterImpl, IStream)

    public:
        using String = std::basic_string<char>;

        StringWriterImpl(String& outputString) :
            m_string(outputString)
        {
        }

    private:
        size_t getPosition() const override
        {
            return m_string.size();
        }

        size_t setPosition(io::OffsetOrigin, int64_t) override
        {
            MY_DEBUG_CHECK("StringWriterImpl::setOffset is not implemented");
            return 0;
        }

        bool canSeek() const override
        {
            return true;
        }

        bool canRead() const override
        {
            return false;
        }

        bool canWrite() const override
        {
            return true;
        }

        Result<size_t> write(const std::byte* ptr, size_t count) override
        {
            constexpr size_t ReserveBlockSize = 16;

            if ((count < ReserveBlockSize) && (m_string.capacity() < m_string.size() + ReserveBlockSize))
            {
                m_string.reserve(m_string.size() + ReserveBlockSize);
            }

            m_string.append(reinterpret_cast<const char*>(ptr), count);
            return count;
        }

        void flush() override
        {
        }

        String& m_string;
    };

    /**
     */
    template <typename StreamImpl>
    requires(std::is_base_of_v<io::IStream, StreamImpl>)
    class InplaceStreamHolder
    {
    public:
        const io::StreamPtr& getStream() const
        {
            MY_DEBUG_CHECK(m_stream);
            return (m_stream);
        }

        operator io::IStream&() const
        {
            return *getStream();
        }

    protected:
        template <typename... Args>
        void createStream(Args&&... args)
        {
            MY_DEBUG_FATAL(!m_stream);
            m_stream = rtti::createInstanceInplace<StreamImpl, io::IStream>(m_inplaceMemBlock, std::forward<Args>(args)...);
        }

    protected:
        io::StreamPtr m_stream;

    private:
        rtti::InstanceInplaceStorage<StreamImpl> m_inplaceMemBlock;
    };

}  // namespace my::io_detail

namespace my::io
{
    class InplaceStringWriter : public io_detail::InplaceStreamHolder<io_detail::StringWriterImpl>
    {
    public:
        InplaceStringWriter(std::string& outputString)
        {
            this->createStream(outputString);
        }
    };
}  // namespace my::io
