// #my_engine_source_file

// #include <cstddef>
#include "./buffer_test_utils.h"
#include "my/async/task.h"
#include "my/diag/assert.h"
#include "my/memory/buffer.h"
#include "my/rtti/rtti_impl.h"
#include "my/test/helpers/runtime_guard.h"
#include "my/utils/functor.h"

using namespace testing;

namespace my::test {
template <typename T>
static testing::AssertionResult checkBufferDefaultContent(const T& buffer, size_t offset = 0, size_t contentOffset = 0, std::optional<size_t> size = std::nullopt)
{
    static_assert(
        std::is_same_v<T, Buffer> ||
        std::is_same_v<T, ReadOnlyBuffer> ||
        std::is_same_v<T, BufferView>);

    const size_t fillSize = !size ? (buffer.size() - offset) : *size;

    MY_ASSERT(offset + fillSize <= buffer.size());

    const uint8_t* const charPtr = reinterpret_cast<const uint8_t*>(buffer.data());

    for (size_t i = offset; i < fillSize; ++i)
    {
        const uint8_t expectedValue = static_cast<uint8_t>((i + contentOffset) % std::numeric_limits<uint8_t>::max());

        const uint8_t bufferValue = charPtr[i];

        if (bufferValue != expectedValue)
        {
            return testing::AssertionFailure() << std::format("Value mismatch at position [{}]. Expected [{} , {}], but [{}, '{}']", i, static_cast<int>(expectedValue), expectedValue, static_cast<int>(bufferValue), bufferValue);
        }
    }

    return testing::AssertionSuccess();
}

/// <summary>
///
/// </summary>
TEST(TestBuffer, Emptiness)
{
    Buffer emptyBuffer;
    ASSERT_FALSE(emptyBuffer);

    Buffer emptyBuffer2;
    ASSERT_FALSE(emptyBuffer2);

    ReadOnlyBuffer emptyReadonlyBuffer;
    ASSERT_FALSE(emptyReadonlyBuffer);

    // BufferView emptyBufferView;
    // ASSERT_FALSE(emptyBufferView);

    Buffer nonEmptyBuffer(10);
    ASSERT_TRUE(nonEmptyBuffer);
    ASSERT_THAT(BufferUtils::refsCount(nonEmptyBuffer), Eq(1));
    ASSERT_EQ(nonEmptyBuffer.size(), 10);

    ReadOnlyBuffer nonEmptyReadonlyBuffer(nonEmptyBuffer.toReadOnly());
    ASSERT_TRUE(nonEmptyReadonlyBuffer);
    ASSERT_THAT(BufferUtils::refsCount(nonEmptyReadonlyBuffer), Eq(1));
    ASSERT_THAT(nonEmptyReadonlyBuffer.size(), Eq(10));
    ASSERT_FALSE(nonEmptyBuffer);

    /*
            BufferView readOnlyBufferView(std::move(nonEmptyReadonlyBuffer));
            ASSERT_TRUE(readOnlyBufferView);
            ASSERT_THAT(readOnlyBufferView.size(), Eq(10));
            ASSERT_FALSE(nonEmptyReadonlyBuffer);
            */
}

/// <summary>
///
/// </summary>
TEST(TestBuffer, Content)
{
    constexpr size_t TestSize = 100;

    Buffer buffer(TestSize);

    void* basePointer = buffer.data();

    ASSERT_TRUE(buffer);
    ASSERT_EQ(buffer.size(), TestSize);

    fillBufferWithDefaultContent(buffer);

    {
        // SCOPED_TRACE("Check buffer initial content");

        ASSERT_TRUE(checkBufferDefaultContent(buffer));
    }

    ReadOnlyBuffer readOnlyBuffer;

    readOnlyBuffer = std::move(buffer);

    ASSERT_TRUE(readOnlyBuffer);
    ASSERT_FALSE(buffer);

    const void* readOnlyPointer = readOnlyBuffer.data();

    ASSERT_EQ(readOnlyPointer, basePointer);

    {
        // SCOPED_TRACE("Check readonly buffer initial content");

        ASSERT_TRUE(checkBufferDefaultContent(readOnlyBuffer));
    }
}

/// <summary>
///
/// </summary>
TEST(TestBuffer, Modify)
{
    constexpr size_t InitialSize = 50;

    Buffer buffer1(InitialSize);
    fillBufferWithDefaultContent(buffer1);

    buffer1.resize(buffer1.size() * 2);
    {
        // SCOPED_TRACE("Non unique buffer content should be unchanged after growing the size");
        ASSERT_TRUE(checkBufferDefaultContent(buffer1, 0, 0, InitialSize));
    }

    Buffer buffer2(InitialSize);
    fillBufferWithDefaultContent(buffer2);

    buffer2.resize(InitialSize / 2);
    ASSERT_TRUE(checkBufferDefaultContent(buffer2));

    Buffer buffer3(InitialSize);
    fillBufferWithDefaultContent(buffer3);

    void* pointerBeforeResize = buffer3.data();

    // buffer3 has no additional references and its size going smaller, so we expect that its pointer remaining unchanged, only size should be changed.
    buffer3.resize(buffer3.size() / 2);

    void* pointerAfterResize = buffer3.data();

    ASSERT_EQ(pointerBeforeResize, pointerAfterResize);

    {
        // SCOPED_TRACE("Unique buffer content should be unchanged after reducing the size");
        ASSERT_TRUE(checkBufferDefaultContent(buffer3));
    }

    Buffer buffer4(InitialSize);

    fillBufferWithDefaultContent(buffer4);

    buffer4.resize(buffer4.size() * 2);

    {
        // SCOPED_TRACE("Unique buffer content should be unchanged after growing the size");
        ASSERT_TRUE(checkBufferDefaultContent(buffer4, 0, 0, InitialSize));
    }
}

/*
TEST(TestBuffer, Merge)
{
    constexpr size_t InitialSize = 50;

    ReadOnlyBuffer readOnlyBuffer1{};
    ReadOnlyBuffer readOnlyBuffer2{};

    {
        Buffer buffer1(InitialSize);
        Buffer buffer2(InitialSize * 2);

        fillBufferWithDefaultContent(buffer1);
        fillBufferWithDefaultContent(buffer2);

        readOnlyBuffer1 = std::move(buffer1);
        readOnlyBuffer2 = std::move(buffer2);
    }

    {
        std::array<ReadOnlyBuffer, 2> buffers = {readOnlyBuffer1, readOnlyBuffer2};

        Buffer mergedBuffer = mergeBuffers(buffers.begin(), buffers.end());

        ASSERT_EQ(mergedBuffer.size(), readOnlyBuffer1.size() + readOnlyBuffer2.size());

        SCOPED_TRACE("Check merged buffer");
        ASSERT_TRUE(checkBufferDefaultContent(mergedBuffer, 0, readOnlyBuffer1.size()));
        ASSERT_TRUE(checkBufferDefaultContent(BufferView(mergedBuffer.toReadOnly(), static_cast<ptrdiff_t>(readOnlyBuffer1.size()))));
    }

    BufferView bufferView1(readOnlyBuffer1, 0, readOnlyBuffer1.size() / 2);
    BufferView bufferView2(readOnlyBuffer1, readOnlyBuffer1.size() / 2);

    auto mergedView = BufferView::merge(bufferView1, bufferView2);

    ASSERT_EQ(mergedView.getBuffer(), bufferView1.getBuffer());
}
*/

TEST(TestBuffer, Move)
{
    constexpr uint32_t InitialSize = 50;

    Buffer buffer(InitialSize);

    fillBufferWithDefaultContent(buffer);

    const void* initialPtr = buffer.data();

    {
        ReadOnlyBuffer readOnlyBuffer = buffer.toReadOnly();

        ASSERT_FALSE(buffer);
        ASSERT_TRUE(readOnlyBuffer);
        ASSERT_EQ(readOnlyBuffer.data(), initialPtr);

        buffer = readOnlyBuffer.toBuffer();

        ASSERT_TRUE(buffer);
        ASSERT_FALSE(readOnlyBuffer);
        ASSERT_EQ(buffer.data(), initialPtr);
    }

#if 0  // TODO: BufferView
        {
            const auto viewSize = buffer.size() / 2;

            BufferView view{buffer.toReadOnly(), 0, viewSize};
            ASSERT_FALSE(buffer);
            ASSERT_TRUE(view);
            ASSERT_EQ(view.data(), initialPtr);

            buffer = view.toBuffer();

            ASSERT_TRUE(buffer);
            ASSERT_FALSE(view);
            ASSERT_EQ(buffer.data(), initialPtr);
            ASSERT_EQ(buffer.size(), viewSize);
        }
#endif
    ASSERT_TRUE(checkBufferDefaultContent(buffer));

    ReadOnlyBuffer readOnlyBuffer = buffer.toReadOnly();
    ReadOnlyBuffer readOnlyBufferCopy = readOnlyBuffer;
    ASSERT_THAT(BufferUtils::refsCount(readOnlyBuffer), Eq(2));

    // Trying to move buffer that have references.
    // Internally copy operation should be performed, but in any case readOnlyBuffer will be released.
    // So test expects that there is only one one reference to the original buffer.
    buffer = readOnlyBuffer.toBuffer();
    ASSERT_FALSE(readOnlyBuffer);
    ASSERT_THAT(BufferUtils::refsCount(readOnlyBufferCopy), Eq(1));

    ASSERT_THAT(buffer.size(), Eq(readOnlyBufferCopy.size()));
    ASSERT_NE(buffer.data(), initialPtr);

    ASSERT_TRUE(checkBufferDefaultContent(buffer));
}

TEST(TestBuffer, Copy)
{
    constexpr uint32_t InitialSize = 50;

    Buffer buffer(InitialSize);
    fillBufferWithDefaultContent(buffer);

    Buffer buffer2 = BufferUtils::copy(buffer);
    ASSERT_TRUE(checkBufferDefaultContent(buffer2));

    ASSERT_NE(buffer.data(), buffer2.data());
    ASSERT_EQ(buffer.size(), buffer2.size());

    // TODO: BufferView
    // Buffer buffer3 = BufferUtils::copy(BufferView{buffer2.toReadOnly()});
    Buffer buffer3 = BufferUtils::copy(buffer2);
    ASSERT_TRUE(checkBufferDefaultContent(buffer3));

    ASSERT_NE(buffer.data(), buffer3.data());
    ASSERT_EQ(buffer.size(), buffer3.size());
}

#if 0  // TODO: BufferView
    TEST(TestBuffer, View)
    {
        constexpr size_t InitialSize = 100;

        const auto initializeView = [size = InitialSize]() -> BufferView
        {
            Buffer buffer{size};

            fillBufferWithDefaultContent(buffer);

            return {buffer.toReadOnly()};
        };

        BufferView view1 = initializeView();

        ASSERT_EQ(view1.size(), InitialSize);
        ASSERT_TRUE(checkBufferDefaultContent(view1));

        BufferView view2 = view1;  // copy constructor;
        ASSERT_EQ(view2.size(), view1.size());
        ASSERT_EQ(view2.data(), view1.data());

        const size_t view3Offset = view2.size() / 2;

        BufferView view3{view2, view3Offset};
        ASSERT_EQ(view3.size(), view2.size() - view3Offset);
        ASSERT_TRUE(checkBufferDefaultContent(view3, 0, view3Offset));

        const size_t view4Offset = view3.size() / 2;

        BufferView view4{view3, view4Offset};
        ASSERT_EQ(view4.size(), view3.size() - view4Offset);
        ASSERT_TRUE(checkBufferDefaultContent(view4, 0, view3Offset + view4Offset));
        ASSERT_EQ(view3.data() + view4Offset, view4.data());
    }
#endif

TEST(TestBuffer, Resize)
{
    GTEST_SKIP_("TODO: Implement me !");
}


TEST(TestBuffer, InternalStorage)
{
    constexpr size_t InitialSize = 100;

    Buffer buffer(InitialSize);

    fillBufferWithDefaultContent(buffer);

    auto storage = BufferStorage::takeOut(std::move(buffer));

    ASSERT_FALSE(buffer);
    ASSERT_THAT(BufferStorage::getClientSize(storage), Eq(InitialSize));

    auto restoredBuffer = BufferStorage::bufferFromHandle(storage);
    ASSERT_THAT(restoredBuffer.size(), Eq(InitialSize));
    ASSERT_THAT(BufferUtils::refsCount(restoredBuffer), Eq(1));
    ASSERT_TRUE(checkBufferDefaultContent(buffer));
}

TEST(TestBuffer, BlockSize)
{
    constexpr size_t Sizes[] = {64, 128, 256, 512, 1500, 2000, 4096};
    constexpr size_t SizesCount = std::end(Sizes) - std::begin(Sizes);
    constexpr size_t IterCount = 250;
    constexpr size_t TotalBuffers = IterCount * SizesCount;
    constexpr size_t ExpectedTotalBytes = std::accumulate(std::begin(Sizes), std::end(Sizes), 0) * IterCount;

    std::vector<Buffer> buffers;
    buffers.reserve(TotalBuffers);

    for (size_t i = 0; i < IterCount; ++i)
    {
        for (const size_t size : Sizes)
        {
            buffers.emplace_back(size);
            memset(buffers.back().data(), 0, buffers.back().size());
        }
    }

    const size_t actualTotalBytes = std::accumulate(buffers.begin(), buffers.end(), size_t{0}, [](size_t accum, const Buffer& buf)
    {
        return buf.size() + accum;
    });

    ASSERT_EQ(actualTotalBytes, ExpectedTotalBytes);
}

#if 0  // TODO: BufferView
    TEST(TestBuffer, Concurrent)
    {
        using namespace my::async;

        constexpr size_t InitialSize = 100;
        constexpr size_t ConcurrentCount = 10;

        const auto runtimeGuard = RuntimeGuard::create();

        Buffer buffer(InitialSize);
        ReadOnlyBuffer readBuffer = buffer.toReadOnly();

        std::vector<Task<>> tasks;

        for(size_t i = 0; i < ConcurrentCount; ++i)
        {
            tasks.emplace_back(async::run([](ReadOnlyBuffer readBuffer) mutable
                                          {
                                              for(size_t x = 0; x < 90; ++x)
                                              {
                                                  [[maybe_unused]]
                                                  BufferView temp{readBuffer, x};
                                              }
                                          },
                                          Executor::getDefault(), readBuffer));
        }

        for(auto& t : tasks)
        {
            async::wait(t);
        }
    }
#endif

}  // namespace my::test
