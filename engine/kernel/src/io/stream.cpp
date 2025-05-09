// #my_engine_source_file

#include "my/io/stream.h"

#include "my/memory/buffer.h"

namespace my::io
{
    Result<size_t> copyFromStream(void* dst, size_t size, IStream& src)
    {
        if(size == 0)
        {
            return 0;
        }

        MY_DEBUG_CHECK(dst);
        if(!dst)
        {
            return MakeError("Invalid dst");
        }

        size_t readOffset = 0;

        for(; readOffset < size;)
        {
            const size_t readCount = size - readOffset;
            const auto readResult = src.read(reinterpret_cast<std::byte*>(dst) + readOffset, readCount);
            CheckResult(readResult);

            if(*readResult == 0)
            {
                break;
            }

            readOffset += *readResult;
            MY_DEBUG_CHECK(readOffset <= size);
        }

        return readOffset;
    }
    
    Result<size_t> copyFromStream(IStream& dst, size_t size, IStream& src)
    {
        MY_DEBUG_CHECK(dst.canWrite());
        MY_DEBUG_CHECK(src.canRead());

        if(size == 0)
        {
            return 0;
        }

        size_t readOffset = 0;
        Buffer buffer(size);
        for(; readOffset < size;)
        {
            const size_t readCount = size - readOffset;
            const auto readResult = src.read(reinterpret_cast<std::byte*>(buffer.data()) + readOffset, readCount);
            CheckResult(readResult);

            if(*readResult == 0)
            {
                break;
            }

            readOffset += *readResult;
            MY_DEBUG_CHECK(readOffset <= size);
        }

        return *dst.write(buffer.data(), size);
    }

    Result<size_t> copyStream(IStream& dst, IStream& src)
    {
        MY_DEBUG_CHECK(dst.canWrite());
        MY_DEBUG_CHECK(src.canRead());

        constexpr size_t BlockSize = 4096;

        Buffer buffer(BlockSize);
        size_t totalRead = 0;

        do
        {
            auto readResult = src.read(buffer.data(), BlockSize);
            CheckResult(readResult);

            const size_t actualRead = *readResult;
            totalRead += actualRead;

            if(actualRead < BlockSize)
            {
                buffer.resize(actualRead);
                const auto writeResult = dst.write(buffer.data(), buffer.size());
                CheckResult(writeResult);
                break;
            }

            const auto writeResult = dst.write(buffer.data(), buffer.size());
            CheckResult(writeResult);

        } while(true);

        return totalRead;
    }
}  // namespace my::io
