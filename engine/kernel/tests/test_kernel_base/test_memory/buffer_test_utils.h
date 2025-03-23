// #my_engine_source_header

#pragma once
#include <gtest/gtest.h>

#include <optional>

#include "my/memory/buffer.h"

namespace my::test
{

    void fillBufferWithDefaultContent(Buffer& buffer, size_t offset = 0, std::optional<size_t> size = std::nullopt);

    Buffer createBufferWithDefaultContent(size_t size);

    testing::AssertionResult buffersEqual(const BufferView& buffer1, const BufferView& buffer2);

}  // namespace my::test