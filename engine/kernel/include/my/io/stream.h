// #my_engine_source_file
#pragma once

#include "my/io/io_constants.h"
#include "my/kernel/kernel_config.h"
#include "my/rtti/ptr.h"
#include "my/rtti/rtti_object.h"
#include "my/utils/result.h"

/**
 * @brief Provides base interfaces for stream representations and utility functions for stream operations.
 */

namespace my::io
{
    /**
     * @struct IStreamBase
     * @brief Base interface for any stream representation.
     *
     * `IStreamBase` defines the basic operations for interacting with a stream, including querying and setting the current position.
     */
    struct MY_ABSTRACT_TYPE IStreamBase : IRefCounted
    {
        MY_INTERFACE(my::io::IStreamBase, IRefCounted)

        // using Ptr = my::Ptr<IStreamBase>; /**< A smart pointer type for `IStreamBase`. */

        /**
         * @brief Returns the current stream position if supported.
         * @return The current position of the stream.
         */
        virtual size_t getPosition() const = 0;

        /**
         * @brief Sets a new stream position as an offset from a specified origin.
         * @param origin The offset origin: current position, beginning, or end of the stream.
         * @param offset The number of bytes to move the stream pointer. Positive values move the pointer forward, and negative values move it backward.
         * @return The new position of the stream if the operation is supported.
         */
        virtual size_t setPosition(OffsetOrigin origin, int64_t offset) = 0;

        /**
         * @brief Flushes the stream, ensuring all buffered data is written.
         */
        virtual void flush() = 0;

        virtual bool canSeek() const = 0;

        virtual bool canRead() const = 0;

        virtual bool canWrite() const = 0;
    };

    using StreamBasePtr = my::Ptr<IStreamBase>;

    /**
     * @brief Interface for reading operations on a stream.
     *
     * `IStreamReader` extends `IStreamBase` and provides methods for reading data from the stream.
     */
    struct MY_ABSTRACT_TYPE IStream : IStreamBase
    {
        MY_INTERFACE(my::io::IStream, IStreamBase)

        /**
         * @brief Reads data from the stream.
         * @param buffer A pointer to the buffer where the read data will be stored.
         * @param count The number of bytes to read.
         * @return A `Result` containing the number of bytes read.
         */
        virtual Result<size_t> read([[maybe_unused]] std::byte* buffer, [[maybe_unused]] size_t count)
        {
            MY_FAILURE("Method not implemented");
            return MakeError("Not implemented");
        }

        /**
         * @brief Writes data to the stream.
         * @param buffer A pointer to the buffer containing the data to be written.
         * @param count The number of bytes to write.
         * @return A `Result` containing the number of bytes written.
         */
        virtual Result<size_t> write([[maybe_unused]] const std::byte* buffer, [[maybe_unused]] size_t count)
        {
            MY_FAILURE("Method not implemented");
            return MakeError("Not implemented");
        }
    };

    using StreamPtr = my::Ptr<IStream>;

    /**
     * @brief Copies data from a reader to a buffer.
     * @param dst A pointer to the destination buffer.
     * @param size The number of bytes to copy.
     * @param src The `IStreamReader` instance to read from.
     * @return A `Result` containing the number of bytes copied.
     */
    MY_KERNEL_EXPORT
    Result<size_t> copyFromStream(void* dst, size_t size, IStream& src);

    /**
     * @brief Copies data from a reader to a writer.
     * @param dst The `IStreamWriter` instance to write to.
     * @param size The number of bytes to copy.
     * @param src The `IStreamReader` instance to read from.
     * @return A `Result` containing the number of bytes copied.
     */
    MY_KERNEL_EXPORT
    Result<size_t> copyFromStream(IStream& dst, size_t size, IStream& src);

    /**
     * @brief Copies an entire stream from a reader to a writer.
     * @param dst The `IStreamWriter` instance to write to.
     * @param src The `IStreamReader` instance to read from.
     * @return A `Result` containing the number of bytes copied.
     */
    MY_KERNEL_EXPORT
    Result<size_t> copyStream(IStream& dst, IStream& src);
}  // namespace my::io
