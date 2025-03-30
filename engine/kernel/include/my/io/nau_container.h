// #my_engine_source_file

#pragma once

#include "my/io/memory_stream.h"
#include "my/serialization/runtime_value.h"

/**
 * @brief Provides functions for writing and reading container headers for serialization.
 */

namespace my::io
{
    /**
     * @brief Writes the header for a container to the output stream.
     *
     * This function writes metadata about a container, including its type and data, to the provided output stream.
     * The header is used during deserialization to correctly interpret the container data.
     *
     * @param outputStream A smart pointer to the `IStreamWriter` used for writing the header.
     * @param kind A string view representing the type of the container.
     * @param containerData A shared pointer to `RuntimeValue` containing the container data.
     */
    MY_KERNEL_EXPORT
    void writeContainerHeader(IStreamWriter::Ptr outputStream, std::string_view kind, const RuntimeValuePtr& containerData);

    /**
     * @brief Reads the header for a container from the input stream.
     *
     * This function reads metadata about a container from the provided input stream. The header includes the type of the container
     * and a size indicating the offset of the header data. This information is used to correctly deserialize the container data.
     *
     * @param stream A smart pointer to the `IStreamReader` used for reading the header.
     * @return A `Result` containing a tuple with:
     *         - `RuntimeValuePtr`: A shared pointer to `RuntimeValue` representing the container data.
     *         - `size_t`: The offset of the header data.
     */
    MY_KERNEL_EXPORT
    Result<std::tuple<RuntimeValuePtr, size_t>> readContainerHeader(IStreamReader::Ptr stream);
}  // namespace my::io
