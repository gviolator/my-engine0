// #my_engine_source_file
#pragma once

#include <atomic>
#include <filesystem>
#include <mutex>

#include "my/io/file_system.h"
#include "my/io/stream.h"
#include "my/rtti/rtti_impl.h"

namespace my::io
{
    class WinFile final : public  IFile, public IMemoryMappableObject, public INativeFile, public io_detail::IFileInternal
    {
        MY_REFCOUNTED_CLASS(WinFile, IFile, IMemoryMappableObject, INativeFile, io_detail::IFileInternal)
    public:
        WinFile(const WinFile&) = delete;
        WinFile(const std::filesystem::path&, AccessModeFlag accessMode, OpenFileMode openMode, DWORD attributes = FILE_ATTRIBUTE_NORMAL);

        virtual ~WinFile();

        bool supports(FileFeature) const final;

        bool isOpened() const final;

        StreamBasePtr createStream(std::optional<AccessModeFlag>) final;

        AccessModeFlag getAccessMode() const override;

        size_t getSize() const override;

        FsPath getPath() const override;

        void* memMap(size_t offset = 0, size_t count = 0) override;

        void memUnmap(const void*) override;

        void setVfsPath(io::FsPath path) override;

        std::filesystem::path getNativePath() const override;

    private:
        FsPath m_vfsPath;
        const AccessModeFlag m_accessMode;
        HANDLE m_fileHandle = INVALID_HANDLE_VALUE;
        HANDLE m_fileMappingHandle = nullptr;
        unsigned m_fileMappingCounter = 0;
        void* m_mappedPtr = nullptr;
        std::mutex m_mutex;
    };

    // class WinFileStreamBase
    // {
    // protected:
    //     WinFileStreamBase(HANDLE fileHandle);
    //     ~WinFileStreamBase();

    //     inline HANDLE getFileHandle() const
    //     {
    //         MY_DEBUG_CHECK(isOpened());
    //         return m_fileHandle;
    //     }

    //     inline bool isOpened() const
    //     {
    //         return m_fileHandle != INVALID_HANDLE_VALUE;
    //     }

    //     size_t getPositionInternal() const;

    //     size_t setPositionInternal(OffsetOrigin, int64_t);

    // private:
    //     const HANDLE m_fileHandle;
    // };


    class WinFileStream final : public virtual IStream
    {
        MY_REFCOUNTED_CLASS(my::io::WinFileStream,  IStream)
    public:
        WinFileStream(HANDLE fileHandle);
        WinFileStream(const std::filesystem::path&, AccessModeFlag accessMode, OpenFileMode openMode);
        ~WinFileStream();

        size_t getPosition() const override;
        size_t setPosition(OffsetOrigin, int64_t) override;

        Result<size_t> read(std::byte*, size_t count) override;
        Result<size_t> write(const std::byte* buffer, size_t count) override;

        void flush() override;
        bool canSeek() const override;
        bool canRead() const override;
        bool canWrite() const override;

        HANDLE getFileHandle() const
        {
            MY_DEBUG_CHECK(isOpened());
            return m_fileHandle;
        }

        bool isOpened() const
        {
            return m_fileHandle != INVALID_HANDLE_VALUE;
        }

    private:
        const HANDLE m_fileHandle;
    };

    // class WinFileStreamWriter final : public WinFileStreamBase,
    //                                   public virtual IStreamWriter
    // {
    //     MY_REFCOUNTED_CLASS(my::io::WinFileStreamWriter, IStreamWriter)
    // public:
    //     using WinFileStreamBase::isOpened;

    //     WinFileStreamWriter(HANDLE fileHandle);
    //     WinFileStreamWriter(const wchar_t* path, AccessModeFlag accessMode, OpenFileMode openMode);

    //     size_t getPosition() const override;

    //     size_t setPosition(OffsetOrigin, int64_t) override;

    //     Result<size_t> write(const std::byte*, size_t count) override;

    //     void flush() override;
    // };

/*
    class WinFileReader : public virtual WinFileBase, public IStreamReader
    {
        MY_REFCOUNTED_CLASS(my::io::WinFileReader, WinFileBase, IStreamReader)
    
    public:
        WinFileReader(const char* path, AccessModeFlag accessMode, OpenFileMode openMode, DWORD attributes);

        bool supports(FileFeature) const final;

        size_t getOffset() const final;

        size_t setOffset(OffsetOrigin, size_t) final;

        Result<size_t> read(std::byte*, size_t count) final;
    };
*/
/*
    class WinFileWriter : public virtual WinFileReader, public IStreamWriter
    {
        MY_REFCOUNTED_CLASS(my::io::WinFileWriter, WinFileReader, IStreamWriter)

    public:

        WinFileWriter (const char* path, AccessModeFlag accessMode, OpenFileMode openMode, DWORD attributes);

        Result<> write(const std::byte*, size_t count) override;

        void flush() override;
    };
    */
    //class WinFileReaderWriter : public virtual WinFileReader, public virtual WinFileWriter
    //{
    //    MY_REFCOUNTED_CLASS(my::io::WinFileReaderWriter, WinFileReader, WinFileWriter)

    //public:

    //    WinFileReaderWriter (const char* path, AccessModeFlag accessMode, OpenFileMode openMode, DWORD attributes);

    //};

    // class WinAsyncFileReader : public WinFileBase, public IAsync
    // {
    //     MY_REFCOUNTED_CLASS(my::io::WinAsyncFileReader, WinFileBase
    // };

    // class WinAsyncFileWriter : public WinAsyncFileReader
    // {

    // };

    // class WinFileReader : public IStreamReader
    // {
    // };

    //IFile::Ptr createNativeFileInternal(const char* path, AccessModeFlag accessMode, OpenFileMode openMode);

}  // namespace my::io
