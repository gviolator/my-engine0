// #my_engine_source_file

#pragma once

/**
 * @brief Provides definitions for the `IMemoryStream` interface and functions to create memory streams.
 */

#include <optional>
#include <span>

#include "my/io/stream.h"
#include "my/memory/buffer.h"

namespace my::io
{
    /**
     * @struct IMemoryStream
     * @brief An abstract interface for in-memory streams that support both reading and writing.
     *
     * `IMemoryStream` provides methods for accessing and manipulating data in memory. It inherits from both `IStreamReader` and `IStreamWriter`.
     */
    struct MY_ABSTRACT_TYPE IMemoryStream : IStream
    {
        MY_INTERFACE(my::io::IMemoryStream, IStream)

        /**
         * @brief Gets a span of the memory buffer.
         * @param offset The starting offset from which to obtain the span. Defaults to 0.
         * @param size The size of the span to obtain. If not provided, the span will include all available data from the offset.
         * @return An `std::span` representing the portion of the memory buffer.
         */
        virtual std::span<const std::byte> getBufferAsSpan(size_t offset = 0, std::optional<size_t> size = std::nullopt) const = 0;
    };

    using MemoryStreamPtr = my::Ptr<IMemoryStream>;

    /**
     * @brief Creates a new in-memory stream with specified access modes.
     * @param accessMode The access modes for the stream (e.g., read and/or write). Defaults to `AccessMode::Write | AccessMode::Read`.
     * @param allocator An optional memory allocator to use. Defaults to `nullptr`.
     * @return A smart pointer to the created `IMemoryStream` instance.
     */
    MY_KERNEL_EXPORT
    MemoryStreamPtr createMemoryStream(AccessModeFlag accessMode = AccessMode::Write | AccessMode::Read, IMemAllocator* allocator = nullptr);

    /**
     * @brief Creates a read-only in-memory stream from a given buffer.
     * @param buffer The buffer to use for the stream.
     * @param allocator An optional memory allocator to use. Defaults to `nullptr`.
     * @return A smart pointer to the created `IMemoryStream` instance.
     */
    MY_KERNEL_EXPORT
    MemoryStreamPtr createReadonlyMemoryStream(std::span<const std::byte> buffer, IMemAllocator* allocator = nullptr);

    /**
     * @brief Creates a new in-memory stream from a `Buffer` with specified access modes.
     * @param buffer The `Buffer` to initialize the stream with.
     * @param accessMode The access modes for the stream (e.g., read and/or write). Defaults to `AccessMode::Write | AccessMode::Read`.
     * @param allocator An optional memory allocator to use. Defaults to `nullptr`.
     * @return A smart pointer to the created `IMemoryStream` instance.
     */
    MY_KERNEL_EXPORT
    MemoryStreamPtr createMemoryStream(Buffer buffer, AccessModeFlag accessMode = AccessMode::Write | AccessMode::Read, IMemAllocator* allocator = nullptr);
}  // namespace my::io
