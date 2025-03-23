// #my_engine_source_header

#pragma once

#include "my/meta/class_info.h"
#include "EASTL/vector.h"

/**
 * @brief Defines structures for representing blob data, file entries in asset packs, and asset pack index data.
 */

namespace my::io
{
    /**
     * @struct BlobData
     * @brief Represents a data blob with size and offset information.
     */
    struct BlobData
    {
        size_t size;   ///< Size of the blob.
        size_t offset; ///< Offset of the blob within a asset pack.

#pragma region Class Info
        MY_CLASS_FIELDS(
            CLASS_FIELD(size),
            CLASS_FIELD(offset))
#pragma endregion
    };

    /**
     * @struct AssetPackFileEntry
     * @brief Represents a file entry within an asset pack.
     */
    struct AssetPackFileEntry
    {
        std::string filePath;            ///< Path to the file within the asset pack.
        std::string contentCompression;  ///< Compression method used for the content, if any.
        size_t clientSize;                 ///< Size of the file without compression.
        BlobData blobData;                 ///< Blob data associated with this file entry.

#pragma region Class Info
        MY_CLASS_FIELDS(
            CLASS_FIELD(filePath),
            CLASS_FIELD(contentCompression),
            CLASS_FIELD(clientSize),
            CLASS_FIELD(blobData))
#pragma endregion
    };

    /**
     * @struct AssetPackIndexData
     * @brief Represents index data for an asset pack, including version and description.
     */
    struct AssetPackIndexData
    {
        std::string version;             ///< Version of the asset pack.
        std::string description;         ///< Description of the asset pack.

        std::vector<AssetPackFileEntry> content; ///< List of file entries within the asset pack.

#pragma region Class Info
        MY_CLASS_FIELDS(
            CLASS_FIELD(version),
            CLASS_FIELD(description),
            CLASS_FIELD(content))
#pragma endregion
    };
} // namespace my::io
