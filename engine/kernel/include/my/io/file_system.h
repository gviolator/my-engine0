// #my_engine_source_file

#pragma once

#include <filesystem>
#include <optional>

#include "my/io/fs_path.h"
#include "my/io/io_constants.h"
#include "my/io/stream.h"
#include "my/kernel/kernel_config.h"
#include "my/rtti/rtti_object.h"

/**
 * @brief Provides definitions for file system-related interfaces and classes.
 */

namespace my::io
{
    /**
     * @enum FsEntryKind
     * @brief Represents the kind of a file system entry.
     */
    enum class FsEntryKind
    {
        File,      ///< Represents a file.
        Directory  ///< Represents a directory.
    };

    /**
     * @struct FsEntry
     * @brief Represents an entry in the file system.
     */
    struct FsEntry
    {
        FsPath path;               ///< Path to the entry.
        FsEntryKind kind;          ///< Type of the entry (file or directory).
        size_t size = 0;           ///< Size of the entry (relevant for files).
        size_t lastWriteTime = 0;  ///< Last write time of the entry.

        /**
         * @brief Checks if the entry is valid.
         * @return True if the entry is valid, otherwise false.
         */
        explicit operator bool() const
        {
            return !isEmpty();
        }

        /**
         * @brief Checks if the entry is empty.
         * @return True if the entry is empty, otherwise false.
         */
        bool isEmpty() const
        {
            return path.isEmpty();
        }

        /**
         * @brief Compares two file system entries.
         * @param entry1 First entry to compare.
         * @param entry2 Second entry to compare.
         * @return Comparison result.
         */
        friend auto operator<=>(const FsEntry& entry1, const FsEntry& entry2)
        {
            return entry1.path <=> entry2.path;
        }
    };

    /**
     * @struct IMemoryMappableObject
     * @brief Interface for objects that can be memory-mapped.
     */
    struct MY_ABSTRACT_TYPE IMemoryMappableObject : virtual IRefCounted
    {
        MY_INTERFACE(my::io::IMemoryMappableObject, IRefCounted)

        /**
         * @brief Maps a portion of the object into memory.
         * @param offset Offset from which to start mapping.
         * @param count Number of bytes to map.
         * @return Pointer to the mapped memory.
         */
        virtual void* memMap(size_t offset = 0, size_t count = 0) = 0;

        /**
         * @brief Unmaps a previously mapped portion of the object from memory.
         * @param ptr Pointer to the mapped memory.
         */
        virtual void memUnmap(const void*) = 0;
    };

    /**
     * @struct IFile
     * @brief Interface for file operations.
     */
    struct MY_ABSTRACT_TYPE IFile : virtual IRefCounted
    {
        MY_INTERFACE(my::io::IFile, IRefCounted)

        using Ptr = my::Ptr<IFile>;

        /**
         * @enum FileFeature
         * @brief Features supported by the file.
         */
        enum class FileFeature
        {
            AsyncStreaming,  ///< Supports asynchronous streaming.
            MemoryMapping    ///< Supports memory mapping.
        };

        /**
         * @brief Checks if a specific file feature is supported.
         * @param feature Feature to check.
         * @return True if the feature is supported, otherwise false.
         */
        virtual bool supports(FileFeature) const = 0;

        /**
         * @brief Checks if the file is currently opened.
         * @return True if the file is opened, otherwise false.
         */
        virtual bool isOpened() const = 0;

        /**
         * @brief Creates a stream for the file.
         * @param accessMode Optional access mode flag.
         * @return Pointer to the created stream.
         */
        virtual StreamBasePtr createStream(std::optional<AccessModeFlag> = std::nullopt) = 0;

        /**
         * @brief Gets the access mode of the file.
         * @return Access mode flag.
         */
        virtual AccessModeFlag getAccessMode() const = 0;

        /**
         * @brief Gets the size of the file.
         * @return Size of the file.
         */
        virtual size_t getSize() const = 0;

        /**
         * @brief Gets the path of the file.
         * @return File path.
         */
        virtual FsPath getPath() const = 0;
    };

    /**
     * @struct INativeFile
     * @brief Interface for native file operations.
     */
    struct MY_ABSTRACT_TYPE INativeFile : virtual IRttiObject
    {
        MY_INTERFACE(my::io::INativeFile, IRttiObject)

        /**
         * @brief Gets the native path of the file.
         * @return Native file path as a string.
         */
        virtual std::filesystem::path getNativePath() const = 0;
    };

    /**
     * @struct MemoryMap
     * @brief RAII wrapper for memory-mapped files.
     */
    struct MemoryMap
    {
        IMemoryMappableObject& file;  ///< Reference to the memory-mappable object.
        void* const ptr;              ///< Pointer to the mapped memory.

        MemoryMap() = delete;
        MemoryMap(const MemoryMap&) = delete;

        /**
         * @brief Constructs a MemoryMap and maps the specified portion of the file.
         * @param infile Memory-mappable object.
         * @param offset Offset to map.
         * @param count Number of bytes to map.
         */
        MemoryMap(IMemoryMappableObject& infile, size_t offset = 0, size_t count = 0) :
            file(infile),
            ptr(file.memMap(offset, count))
        {
            // MY_DEBUG_ASSERT(file.supports(IFile::FileFeature::MemoryMapping));
            MY_DEBUG_ASSERT(ptr != nullptr);
        }

        /**
         * @brief Unmaps the memory upon destruction.
         */
        ~MemoryMap()
        {
            file.memUnmap(ptr);
        }

        /**
         * @brief Converts the MemoryMap to a void pointer.
         * @return Pointer to the mapped memory.
         */
        operator void*() const
        {
            return ptr;
        }
    };

    /**
     * @struct FileSystem
     * @brief Interface for file system operations.
     */
    struct MY_ABSTRACT_TYPE FileSystem : virtual IRefCounted
    {
        MY_INTERFACE(my::io::FileSystem, IRefCounted)

        /**
         * @brief Checks if the file system is read-only.
         * @return True if read-only, otherwise false.
         */
        virtual bool isReadOnly() const = 0;

        /**
         * @brief Checks if a path exists in the file system.
         * @param path Path to check.
         * @param kind Optional type of entry to check.
         * @return True if the path exists, otherwise false.
         */
        virtual bool exists(const FsPath&, std::optional<FsEntryKind> kind = std::nullopt) = 0;

        /**
         * @brief Gets the last write time of a path.
         * @param path Path to query.
         * @return Last write time.
         */
        virtual size_t getLastWriteTime(const FsPath&) = 0;

        /**
         * @brief Opens a file in the file system.
         * @param path Path to the file.
         * @param accessMode Access mode flag.
         * @param openMode Open mode flag.
         * @return Pointer to the opened file.
         */
        [[nodiscard]]
        virtual IFile::Ptr openFile(const FsPath&, AccessModeFlag accessMode, OpenFileMode openMode) = 0;

        /**
         * @typedef OpenDirResult
         * @brief Result of opening a directory iterator.
         */
        using OpenDirResult = Result<std::tuple<void*, FsEntry>>;

        /**
         * @brief Opens a directory iterator.
         * @param path Path to the directory.
         * @return Result containing the iterator state and first directory entry.
         */
        [[nodiscard]]
        virtual OpenDirResult openDirIterator(const FsPath& path) = 0;

        /**
         * @brief Closes a directory iterator.
         * @param iteratorState State of the iterator to close.
         */
        virtual void closeDirIterator(void*) = 0;

        /**
         * @brief Increments the directory iterator.
         * @param iteratorState State of the iterator.
         * @return Next directory entry.
         */
        virtual FsEntry incrementDirIterator(void*) = 0;
    };

    using FileSystemPtr = my::Ptr<FileSystem>;

    /**
     * @struct INativeFileSystem
     * @brief Interface for native file system operations.
     */
    struct MY_ABSTRACT_TYPE INativeFileSystem : virtual IRefCounted
    {
        MY_INTERFACE(my::io::INativeFileSystem, IRefCounted)

        /**
         * @brief Resolves a virtual path to a native path.
         * @param path Virtual path to resolve.
         * @return Native path as a wide string.
         */
        virtual std::filesystem::path resolveToNativePath(const FsPath& path) = 0;
    };

    /**
     * @struct IMutableFileSystem
     * @brief Interface for mutable file systems that support file and directory creation/deletion.
     */
    struct MY_ABSTRACT_TYPE IMutableFileSystem : virtual FileSystem
    {
        MY_INTERFACE(my::io::IMutableFileSystem, FileSystem)

        /**
         * @brief Creates a new directory.
         * @param path Path to the directory.
         * @return Result of the operation.
         */
        virtual Result<> createDirectory(const FsPath&) = 0;

        /**
         * @brief Removes a file or directory.
         * @param path Path to the file or directory.
         * @param recursive Whether to remove directories recursively.
         * @return Result of the operation.
         */
        virtual Result<> remove(const FsPath&, bool recursive = false) = 0;
    };

    /**
     * @class DirectoryIterator
     * @brief Iterator for traversing directory entries.
     */
    class MY_KERNEL_EXPORT DirectoryIterator
    {
    public:
        /**
         * @class iterator
         * @brief Iterator for directory entries.
         */
        class MY_KERNEL_EXPORT iterator
        {
        public:
            using iterator_category = std::input_iterator_tag;
            using value_type = FsEntry;
            using difference_type = ptrdiff_t;
            using pointer = const FsEntry*;
            using reference = const FsEntry&;

            iterator();
            iterator(iterator&&);
            bool operator==(const iterator&) const;
            iterator& operator++();
            iterator operator++(int);
            const FsEntry& operator*() const&;
            FsEntry&& operator*() &&;
            const FsEntry* operator->() const;

            iterator& operator=(const iterator&) = delete;
            iterator& operator=(iterator&&) = delete;

        private:
            iterator(DirectoryIterator& parent, FsEntry&&);
            bool isEnd() const;

            DirectoryIterator* m_parent = nullptr;
            FsEntry m_fsEntry;

            friend class DirectoryIterator;
        };

        DirectoryIterator() = default;
        DirectoryIterator(FileSystemPtr fileSystem, FsPath virtualPath);
        DirectoryIterator(const DirectoryIterator&) = delete;
        DirectoryIterator(DirectoryIterator&&) = delete;
        ~DirectoryIterator();

        DirectoryIterator& operator=(const DirectoryIterator&) = delete;
        DirectoryIterator& operator=(DirectoryIterator&&) = delete;

    private:
        iterator start();
        FsEntry increment();

        FileSystemPtr m_fs;
        FsPath m_path;
        void* m_iteratorState = nullptr;

        [[nodiscard]]
        inline friend DirectoryIterator::iterator begin(DirectoryIterator& dirIter) noexcept
        {
            return dirIter.start();
        }

        [[nodiscard]]
        inline friend DirectoryIterator::iterator end(const DirectoryIterator&) noexcept
        {
            return {};
        }
    };

    /**
     * @brief Creates a native file system.
     * @param basePath Base path for the file system.
     * @return Pointer to the created file system.
     */
    MY_KERNEL_EXPORT
    FileSystemPtr createNativeFileSystem(std::filesystem::path basePath, bool readOnly = true);

    /**
     * @brief Creates a zip archive file system.
     * @param stream Stream to read the zip archive from.
     * @param basePath Optional base path within the archive.
     * @return Pointer to the created file system.
     */
    MY_KERNEL_EXPORT
    FileSystemPtr createZipArchiveFileSystem(StreamPtr stream, std::string basePath = {});

    /**
     * @brief Creates a file stream.
     * @param path Path to the file.
     * @param accessMode Access mode for the stream.
     * @param openMode Open mode for the file.
     * @return Pointer to the created stream.
     */
    MY_KERNEL_EXPORT
    StreamBasePtr createNativeFileStream(std::filesystem::path, AccessModeFlag accessMode, OpenFileMode openMode);

}  // namespace my::io

namespace my::io_detail
{
    /**
     * @struct IFileInternal
     * @brief Internal interface for file management.
     */
    struct IFileInternal
    {
        MY_TYPEID(my::io_detail::IFileInternal);

        virtual ~IFileInternal() = default;

        /**
         * @brief Sets the virtual file system path.
         * @param path Virtual file system path.
         */
        virtual void setVfsPath(io::FsPath path) = 0;
    };

}  // namespace my::io_detail
