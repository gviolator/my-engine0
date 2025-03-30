// #my_engine_source_file

#pragma once

#include <filesystem>
#include <EASTL/string.h>

#include "my/kernel/kernel_config.h"
#include "my/utils/enum/enum_reflection.h"

namespace my::io
{
    MY_DEFINE_ENUM_(KnownFolder,
        Temp,
        ExecutableLocation,
        Current,
        UserDocuments,
        UserHome,
        LocalAppData
    );

    /**
     * @brief Generates a native temporary file path with a specified prefix.
     *
     * This function creates a temporary file path that is unique and prefixed with the provided file name. The generated path is
     * suitable for use in file operations where a temporary file is required.
     *
     * @param prefixFileName The prefix to use for the temporary file name. Defaults to "NAU".
     * @return An `std::u8string` representing the generated temporary file path.
     */
    MY_KERNEL_EXPORT
    std::u8string getNativeTempFilePath(std::u8string_view prefixFileName = u8"NAU");

    /**
        @brief Retrieves the full path of a known folder identified by the folder.
        @param folder The folder id.
        @return  UTF-8 string that specifies the path of the known folder.
     */
    MY_KERNEL_EXPORT
    std::filesystem::path getKnownFolderPath(KnownFolder folder);

}  // namespace my::io
