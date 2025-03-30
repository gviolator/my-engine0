// #my_engine_source_file

#pragma once

#include "my/io/file_system.h"
#include "my/kernel/kernel_config.h"
#include "my/rtti/ptr.h"

/**
 * @brief Provides the interface for virtual file systems and related functionalities.
 */

namespace my::io
{
    /**
     * @struct IVirtualFileSystem
     * @brief Interface for a virtual file system that supports mounting and unmounting of other file systems.
     * @details This interface extends both `IMutableFileSystem` and `INativeFileSystem`. It provides functionalities
     *          for managing multiple file systems as a unified virtual file system.
     */
    struct MY_ABSTRACT_TYPE IVirtualFileSystem : IMutableFileSystem,
                                                  INativeFileSystem
    {
        MY_INTERFACE(my::io::IVirtualFileSystem, IMutableFileSystem, INativeFileSystem)

        using Ptr = my::Ptr<IVirtualFileSystem>;  ///< Type alias for a pointer to an `IVirtualFileSystem`.

        /**
         * @brief Mounts a file system to a specified path within the virtual file system.
         * @param path Path at which the file system will be mounted.
         * @param fileSystem Pointer to the file system to mount.
         * @param priority Priority of the mounted file system.
         * @return Result of the operation.
         * @details If multiple file systems are mounted at the same path, the one with the highest priority will be used.
         */
        virtual Result<> mount(const FsPath&, IFileSystem::Ptr, unsigned priority = 1) = 0;

        /**
         * @brief Unmounts a previously mounted file system.
         * @param fileSystem Pointer to the file system to unmount.
         */
        virtual void unmount(IFileSystem::Ptr) = 0;
    };

    /**
     * @brief Creates a new virtual file system.
     * @return Pointer to the created virtual file system.
     */
    MY_KERNEL_EXPORT
    IVirtualFileSystem::Ptr createVirtualFileSystem();

}  // namespace my::io
