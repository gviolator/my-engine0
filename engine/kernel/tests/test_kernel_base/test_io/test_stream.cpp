// #my_engine_source_file

#include "my/io/stream.h"
#include "my/io/memory_stream.h"

using namespace my::io;

namespace my::test
{
    void fillBufferWithDefaultContent(Buffer& buffer, std::optional<size_t> size)
    {
        uint8_t* const charPtr = reinterpret_cast<uint8_t*>(buffer.data());

        for(size_t i = 0; i < size; ++i)
        {
            charPtr[i] = static_cast<uint8_t>(i % std::numeric_limits<uint8_t>::max());
        }
    }

    TEST(TestStreamFunction, CopyStream)
    {
        std::string_view testData = "test data";

        Buffer buffer;
        buffer = fromStringView(testData);

        MemoryStreamPtr srcStream = createMemoryStream(std::move(buffer));
        MemoryStreamPtr dstStream = createMemoryStream();

        const size_t size = *io::copyStream(*dstStream, *srcStream);
        ASSERT_TRUE(size == testData.size());

        Buffer result;
        dstStream->setPosition(OffsetOrigin::Begin, 0);
        dstStream->read(result.append(testData.size()), testData.size()).ignore();
        
        ASSERT_TRUE(memcmp(testData.data(), result.data(), testData.size()) == 0);
    }

    TEST(TestStreamFunction, CopyStream_WithLongData)
    {
        constexpr size_t bufferSize = 1048576;
        Buffer buffer(bufferSize);
        fillBufferWithDefaultContent(buffer, bufferSize);
        std::string_view testData = asStringView(buffer);

        MemoryStreamPtr srcStream = createMemoryStream(std::move(buffer));
        MemoryStreamPtr dstStream = createMemoryStream();

        const size_t size = *io::copyStream(*dstStream, *srcStream);
        ASSERT_TRUE(size == bufferSize);

        Buffer result;
        dstStream->setPosition(OffsetOrigin::Begin, 0);
        dstStream->read(result.append(bufferSize), bufferSize).ignore();

        ASSERT_TRUE(memcmp(testData.data(), result.data(), bufferSize) == 0);
    }

    TEST(TestStreamFunction, CopyStream_CheckCorrectPosition)
    {
        constexpr size_t bufferSize = 516;
        Buffer buffer(bufferSize);
        fillBufferWithDefaultContent(buffer, bufferSize);
        //std::string_view testData = asStringView(buffer);

        MemoryStreamPtr srcStream = createMemoryStream(std::move(buffer));
        MemoryStreamPtr dstStream = createMemoryStream();

        const size_t size = *io::copyStream(*dstStream, *srcStream);
        ASSERT_TRUE(size == bufferSize);

        Buffer result;
        dstStream->setPosition(OffsetOrigin::Begin, 0);
        dstStream->read(result.append(bufferSize), bufferSize).ignore();

        ASSERT_TRUE(dstStream->getPosition() == srcStream->getPosition());
    }
}