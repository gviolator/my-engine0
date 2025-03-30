// #my_engine_source_file


#pragma once
#include <EASTL/string.h>
#include <EASTL/string_view.h>
#include <EASTL/variant.h>

#include <type_traits>

#include "my/dag_ioSys/dag_baseIo.h"
#include "my/dag_ioSys/dag_genIo.h"
#include "my/diag/check.h"
#include "my/io/stream.h"
#include "my/kernel/kernel_config.h"
#include "my/rtti/rtti_impl.h"

namespace my::io_detail
{
    /**
     */
    template <typename Char>
    requires(sizeof(Char) == sizeof(char))
    class StringWriterImpl final : public io::IStreamWriter
    {
        MY_CLASS(StringWriterImpl<Char>, rtti::RCPolicy::StrictSingleThread, IStreamWriter)

    public:
        using String = std::basic_string<Char>;

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

        Result<size_t> write(const std::byte* ptr, size_t count) override
        {
            constexpr size_t ReserveBlockSize = 16;

            if ((count < ReserveBlockSize) && (m_string.capacity() < m_string.size() + ReserveBlockSize))
            {
                m_string.reserve(m_string.size() + ReserveBlockSize);
            }

            m_string.append(reinterpret_cast<const Char*>(ptr), count);
            return count;
        }

        void flush() override
        {
        }

        String& m_string;
    };

    /**
     */
    template <typename StreamImpl, typename StreamApi>
    requires(std::is_base_of_v<StreamApi, StreamImpl>)
    class InplaceStreamHolder
    {
    public:
        const my::Ptr<StreamApi>& getStream() const
        {
            MY_DEBUG_CHECK(m_stream);
            return (m_stream);
        }

        operator StreamApi&() const
        {
            return *getStream();
        }

    protected:
        template <typename... Args>
        void createStream(Args&&... args)
        {
            MY_DEBUG_FATAL(!m_stream);
            m_stream = rtti::createInstanceInplace<StreamImpl, StreamApi>(m_inplaceMemBlock, std::forward<Args>(args)...);
        }

    protected:
        my::Ptr<StreamApi> m_stream;

    private:
        rtti::InstanceInplaceStorage<StreamImpl> m_inplaceMemBlock;
    };

}  // namespace my::io_detail

namespace my::io
{
    template <typename Char = char8_t>
    class InplaceStringWriter : public io_detail::InplaceStreamHolder<io_detail::StringWriterImpl<Char>, IStreamWriter>
    {
    public:
        InplaceStringWriter(std::basic_string<Char>& outputString)
        {
            this->createStream(outputString);
        }
    };

    /**
     */
    class MY_KERNEL_EXPORT GenLoadOverStream : public iosys::IBaseLoad
    {
    public:
        GenLoadOverStream(IStreamReader::Ptr stream, std::string_view targetName = {});
        GenLoadOverStream(const GenLoadOverStream&) = delete;
        GenLoadOverStream(GenLoadOverStream&&) = default;

        GenLoadOverStream& operator=(const GenLoadOverStream&) = delete;
        GenLoadOverStream& operator=(GenLoadOverStream&&) = default;

        void read(void* ptr, int size) override;

        int tryRead(void* ptr, int size) override;

        int tell() override;

        void seekto(int position) override;

        void seekrel(int offset) override;

        const char* getTargetName() override;

    private:
        IStreamReader::Ptr m_stream;
        std::string m_targetName;
    };

}  // namespace my::io
