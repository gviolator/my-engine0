// #my_engine_source_file

#include "my/diag/log_sinks.h"
#include "my/rtti/rtti_impl.h"
#include "my/utils/string_conv.h"

namespace my::diag
{
    class DebugOutStream : public io::IStream
    {
        MY_REFCOUNTED_CLASS(my::diag::DebugOutStream, io::IStream)

        size_t getPosition() const override
        {
            return 0;
        }

        size_t setPosition(io::OffsetOrigin, int64_t) override
        {
            return 0;
        }

        bool canSeek() const override
        {
            return false;
        }

        Result<size_t> write(const std::byte* buffer, size_t count) override
        {
            std::wstring text = strings::utf8ToWString({reinterpret_cast<const char*>(buffer), count});
            if (!text.ends_with(L'\n'))
            {
                text.append(L"\n");
            }

            ::OutputDebugStringW(text.c_str());
            return count;
        }

        void flush() override
        {
        }

        bool canRead() const override
        {
            return false;
        }

        bool canWrite() const override
        {
            return true;
        }
    };

    io::StreamPtr createDebugOutputStream()
    {
        return rtti::createInstance<DebugOutStream>();
    }
}  // namespace my::diag