// #my_engine_source_file

#include "my/io/memory_stream.h"
#include "my/memory/buffer.h"

using namespace testing;

namespace my::test
{
    io::MemoryStreamPtr createMemoryStream(std::string_view testData)
    {
        Buffer buffer = fromStringView(testData);

        return io::createMemoryStream(std::move(buffer));
    }

    TEST(TestMemoryStream, ReadFromStream)
    {
        std::string_view testData = "test data";
        io::MemoryStreamPtr memoryStream = createMemoryStream(testData);

        Buffer resultBuffer;
        const size_t result = *memoryStream->read(resultBuffer.append(testData.size()), testData.size());

        ASSERT_TRUE(result == testData.size());
        ASSERT_THAT(asStringView(resultBuffer), Eq(testData));
    }

    TEST(TestMemoryStream, SetPosition_FromBegin)
    {
        std::string_view testData = "test data";
        io::MemoryStreamPtr memoryStream = createMemoryStream(testData);

        constexpr size_t offset = 5;
        const size_t offsetDataSize = testData.size() - offset;
        memoryStream->setPosition(io::OffsetOrigin::Begin, offset);

        Buffer resultBuffer;
        const size_t result = *memoryStream->read(resultBuffer.append(offsetDataSize), offsetDataSize);

        ASSERT_TRUE(result == offsetDataSize);
        ASSERT_THAT(asStringView(resultBuffer), Eq("data"));
    }

    TEST(TestMemoryStream, SetPosition_FromCurrent)
    {
        std::string_view testData = "test data";
        io::MemoryStreamPtr memoryStream = createMemoryStream(testData);

        constexpr size_t beginOffset = 4;
        memoryStream->setPosition(io::OffsetOrigin::Begin, beginOffset);

        constexpr size_t offset = 1;
        const size_t offsetDataSize = testData.size() - beginOffset - offset;
        memoryStream->setPosition(io::OffsetOrigin::Current, offset);

        Buffer resultBuffer;
        const size_t result = *memoryStream->read(resultBuffer.append(offsetDataSize), offsetDataSize);

        ASSERT_TRUE(result == offsetDataSize);
        ASSERT_THAT(asStringView(resultBuffer), Eq("data"));
    }

    TEST(TestMemoryStream, SetPosition_FromEnd)
    {
        std::string_view testData = "test data";
        io::MemoryStreamPtr memoryStream = createMemoryStream(testData);

        constexpr int64_t offset = 4;
        memoryStream->setPosition(io::OffsetOrigin::End, -offset);

        Buffer resultBuffer;
        const size_t result = *memoryStream->read(resultBuffer.append(offset), offset);

        ASSERT_TRUE(result == offset);
        ASSERT_THAT(asStringView(resultBuffer), Eq("data"));
    }
}  // namespace my::test
