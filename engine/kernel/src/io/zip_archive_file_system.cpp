// #my_engine_source_file

#include "my/io/file_system.h"
#include "my/rtti/rtti_impl.h"
#include "my/utils/scope_guard.h"

#if 0

namespace my::io
{
    class ZipArchiveFileSystem final : public FileSystem
    {
        MY_CLASS_(my::io::ZipArchiveFileSystem, FileSystem)

    public:
        ZipArchiveFileSystem(IStreamReader::Ptr);
        ~ZipArchiveFileSystem();

        bool isReadOnly() const override;
        bool exists(const FsPath&, std::optional<FsEntryKind>) override;
        size_t getLastWriteTime(const FsPath&) override;
        IFile::Ptr openFile(const FsPath&, AccessModeFlag accessMode, OpenFileMode openMode) override;

        OpenDirResult openDirIterator(const FsPath& path) override;
        void closeDirIterator(void*) override;
        FsEntry incrementDirIterator(void*) override;

    private:
        struct ZipFsEntry : io::FsEntry
        {
            ::unz64_file_pos zipPos;
            std::vector<std::byte> buffer;
        };

        struct DirIteratorState
        {
            const std::vector<const ZipFsEntry*> entries;
            const FsPath path;
            size_t currentIndex = 0;


            DirIteratorState(std::vector<const ZipFsEntry*>&& inEntries, FsPath inPath) :
                entries(std::move(inEntries)),
                path(std::move(inPath))
            {}

            FsEntry getCurrentFsEntry() const
            {
                if (entries.size() <= currentIndex)
                {
                    return {};
                }

                const ZipFsEntry& zipEntry = *entries[currentIndex];

                //FsEntry entry = zipEntry;
                //entry.path = path / zipEntry.path;

                return zipEntry;
            }
        };


        static void* zipOpen(void* opaque, const void*, int mode) noexcept
        {
            MY_DEBUG_ASSERT(opaque);
            auto& self = *reinterpret_cast<ZipArchiveFileSystem*>(opaque);

            MY_DEBUG_ASSERT(self.m_stream);
            return self.m_stream.get();
        }

        static int zipClose([[maybe_unused]] void* opaque, [[maybe_unused]] void* StreamBasePtr) noexcept
        {
            MY_DEBUG_ASSERT(opaque);
            MY_DEBUG_ASSERT(StreamBasePtr);

            return 0;
        }

        static uint32_t zipRead(void*, void* StreamBasePtr, void* buf, uint32_t count) noexcept
        {
            auto* const stream = reinterpret_cast<IStreamBase*>(StreamBasePtr);
            MY_DEBUG_ASSERT(stream);

            auto& streamReader = stream->as<IStreamReader&>();

            const auto actualRead = streamReader.read(reinterpret_cast<std::byte*>(buf), static_cast<size_t>(count));
            if(!actualRead)
            {  // report error ?
                return 0;
            }

            return static_cast<uint32_t>(*actualRead);
        }

        static uint32_t zipWrite(void*, void*, const void*, uint32_t) noexcept
        {
            MY_DEBUG_ASSERT("Zip write is not supported");
            return 0;
        }

        static uint64_t zipTell(void*, void* StreamBasePtr) noexcept
        {
            auto* const stream = reinterpret_cast<IStreamBase*>(StreamBasePtr);
            MY_DEBUG_ASSERT(stream);

            return stream->getOffset();
        }

        static long zipSeek(void*, void* StreamBasePtr, uint64_t offset, int origin) noexcept
        {
            auto* const stream = reinterpret_cast<IStreamBase*>(StreamBasePtr);
            MY_DEBUG_ASSERT(stream);

            const auto seekOrigin = EXPR_Block -> OffsetOrigin
            {
                if(origin == ZLIB_FILEFUNC_SEEK_END)
                {
                    return OffsetOrigin::End;
                }
                else if(origin == ZLIB_FILEFUNC_SEEK_CUR)
                {
                    return OffsetOrigin::Current;
                }

                MY_DEBUG_ASSERT(origin == ZLIB_FILEFUNC_SEEK_SET);
                return OffsetOrigin::Begin;
            };

            [[maybe_unused]]
            const auto newOffset = stream->setOffset(seekOrigin, offset);

            return 0;
        }

        static int zipError(void*, void*) noexcept
        {
            return 0;
        }

        static zlib_filefunc64_def_s getZipFileFuncs(void* opaque)
        {
            zlib_filefunc64_def_s fileFuncs{};

            fileFuncs.zopen64_file = zipOpen;
            fileFuncs.zread_file = zipRead;
            fileFuncs.zwrite_file = zipWrite;
            fileFuncs.ztell64_file = zipTell;
            fileFuncs.zseek64_file = zipSeek;
            fileFuncs.zclose_file = zipClose;
            fileFuncs.zerror_file = zipError;
            fileFuncs.opaque = opaque;

            return fileFuncs;
        }

        StreamBasePtr m_stream;
        ::unzFile m_zipArchive = nullptr;

        std::vector<ZipFsEntry> m_index;
        std::mutex m_mutex;

        friend class ArchiveFile;
    };

    ZipArchiveFileSystem::ZipArchiveFileSystem(IStreamReader::Ptr stream) :
        m_stream(std::move(stream))
    {
        auto fileFuncs = getZipFileFuncs(this);
        m_zipArchive = ::unzOpen2_64("internal", &fileFuncs);
        MY_DEBUG_ASSERT(m_zipArchive);
        if(!m_zipArchive)
        {
            return;
        }

        const auto makeForwardSlashPath = [](char* path, size_t pathLen)
        {
            for(size_t i = 0; i < pathLen; ++i)
            {
                if(path[i] == '\\')
                {
                    path[i] = '/';
                }
            }
        };

        m_index.reserve(10);

        constexpr size_t NameLenMax = 500;
        std::array<char, NameLenMax + 1> nameBuffer;

        for(int code = ::unzGoToFirstFile(m_zipArchive); code == UNZ_OK; code = ::unzGoToNextFile(m_zipArchive))
        {
            unz_file_info64 zipFileInfo;
            if(::unzGetCurrentFileInfo64(m_zipArchive, &zipFileInfo, nameBuffer.data(), static_cast<uint16_t>(nameBuffer.size()), nullptr, 0, nullptr, 0) != UNZ_OK)
            {
                break;
            }

            const size_t fileNameLen = static_cast<size_t>(zipFileInfo.size_filename);
            makeForwardSlashPath(nameBuffer.data(), fileNameLen);

            ZipFsEntry& arcEntry = m_index.emplace_back();
            arcEntry.size = static_cast<size_t>(zipFileInfo.uncompressed_size);
            arcEntry.kind = (arcEntry.size > 0) ? FsEntryKind::File : FsEntryKind::Directory;
            arcEntry.lastWriteTime = static_cast<size_t>(zipFileInfo.dos_date);

            std::string_view innerPath = {nameBuffer.data(), fileNameLen};
            if (innerPath.ends_with('/'))
            {
                innerPath.remove_suffix(1);
            }
            
            arcEntry.path = innerPath;

            ::unzGetFilePos64(m_zipArchive, &arcEntry.zipPos);
        }

        std::sort(m_index.begin(), m_index.end());
    }

    class ArchiveFile final : public io::IFile, public io_detail::IFileInternal
    {
        MY_CLASS_(my::io::ArchiveFile, io::IFile, io_detail::IFileInternal)

    public:
        ArchiveFile(my::Ptr<ZipArchiveFileSystem>&& archiveFileSystem, const ZipArchiveFileSystem::ZipFsEntry& fsEntry) :
            m_archiveFileSystem(std::move(archiveFileSystem)),
            m_fsEntry(fsEntry)
        {
        }

        ~ArchiveFile()
        {
        }

        bool supports(FileFeature) const override
        {
            return false;
        }

        bool isOpened() const override
        {
            return true;
        }

        AccessModeFlag getAccessMode() const override
        {
            return AccessMode::Read;
        }

        FsPath getPath() const override
        {
            return m_vfsPath;
        }

        StreamBasePtr createStream(std::optional<AccessModeFlag>) override
        {
            auto result = EXPR_Block->Result<std::tuple<const void*, size_t>>
            {
                const std::lock_guard lock(m_archiveFileSystem->m_mutex);

                if(m_uncompressedBuffer.empty())
                {
                    auto archive = m_archiveFileSystem->m_zipArchive;

                    ::unzGoToFilePos64(archive, &m_fsEntry.zipPos);
                    ::unzOpenCurrentFile(archive);

                    scope_on_leave
                    {
                        ::unzCloseCurrentFile(archive);
                    };

                    m_uncompressedBuffer.resize(m_fsEntry.size);
                    size_t readOffset = 0;

                    constexpr size_t ZipReadMaxBytes = UINT16_MAX;

                    for(; readOffset < m_uncompressedBuffer.size();)
                    {
                        void* const ptr = m_uncompressedBuffer.data() + readOffset;
                        const size_t readCount = std::min(m_uncompressedBuffer.size() - readOffset, ZipReadMaxBytes);
                        const int readResult = ::unzReadCurrentFile(archive, ptr, static_cast<uint32_t>(readCount));

                        MY_DEBUG_ASSERT(readResult >= 0);
                        if(readResult < 0)
                        {
                            return MakeError("Read zip entry error");
                        }
                        if(readResult == 0)
                        {
                            break;
                        }

                        readOffset += static_cast<size_t>(readResult);
                    }

                    if(readOffset < m_uncompressedBuffer.size())
                    {
                        m_uncompressedBuffer.resize(readOffset);
                    }
                }

                return {m_uncompressedBuffer.data(), m_uncompressedBuffer.size()};
            };

            if (!result)
            {
                return nullptr;
            }
            
            const auto [ptr, size] = *result;
    
            return io::createReadOnlyMemoryStream(ptr, size, rtti::Acquire {as<IRefCounted*>()});
        }

        uint64_t getSize() const override
        {
            return m_fsEntry.size;
        }
  
    private:
        void setVfsPath(io::FsPath vfsPath) override
        {
            m_vfsPath = std::move(vfsPath);
        }

        my::Ptr<ZipArchiveFileSystem> m_archiveFileSystem;
        const ZipArchiveFileSystem::ZipFsEntry& m_fsEntry;
        std::vector<std::byte> m_uncompressedBuffer;
        FsPath m_vfsPath;
    };

    ZipArchiveFileSystem::~ZipArchiveFileSystem()
    {
        if(m_zipArchive)
        {
            ::unzClose(m_zipArchive);
        }
    }

    bool ZipArchiveFileSystem::isReadOnly() const
    {
        return true;
    }

    bool ZipArchiveFileSystem::exists(const FsPath& path, [[maybe_unused]] std::optional<FsEntryKind> kind)
    {
        struct OperatorLess
        {
            bool operator()(const FsEntry& fs, const FsPath& path) const
            {
                return fs.path < path;
            }

            bool operator()(const FsPath& path, const FsEntry& fs) const
            {
                return path < fs.path;
            }
        };

        if (path.isEmpty())
        { // root
            return true;
        }


        return std::binary_search(m_index.begin(), m_index.end(), path, OperatorLess{});
    }

    size_t ZipArchiveFileSystem::getLastWriteTime(const FsPath&)
    {
        return 0;
    }

    IFile::Ptr ZipArchiveFileSystem::openFile(const FsPath& path, AccessModeFlag accessMode, OpenFileMode openMode)
    {
        MY_DEBUG_ASSERT(openMode == OpenFileMode::OpenExisting);
        MY_DEBUG_ASSERT(!(accessMode && AccessMode::Write));

        // m_index is not mutable.
        auto iter = std::find_if(m_index.begin(), m_index.end(), [&path](const ZipFsEntry& entry)
                                 {
                                     return entry.path == path;
                                 });

        if(iter == m_index.end())
        {
            return nullptr;
        }

        return rtti::createInstance<ArchiveFile>(rtti::Acquire{this}, *iter);
    }

    FileSystem::OpenDirResult ZipArchiveFileSystem::openDirIterator(const FsPath& path)
    {
        std::vector<const ZipFsEntry*> entries;
        for (const auto& entry : m_index)
        {
            if (entry.path.getParentPath() == path)
            {
                entries.push_back(&entry);
            }
        }

        if (entries.empty())
        {
            return {};
        }

        auto* const state = new DirIteratorState {std::move(entries), path };

        return { state, state->getCurrentFsEntry() };
    }

    void ZipArchiveFileSystem::closeDirIterator(void* statePtr)
    {
        if (statePtr)
        {
            auto* const state = reinterpret_cast<DirIteratorState*>(statePtr);
            delete state;
        }
    }

    FsEntry ZipArchiveFileSystem::incrementDirIterator(void* statePtr)
    {
        MY_DEBUG_ASSERT(statePtr);
        auto* const state = reinterpret_cast<DirIteratorState*>(statePtr);

        if (++state->currentIndex >= state->entries.size())
        {
            return {};
        }

        return state->getCurrentFsEntry();
    }

    FileSystemPtr createZipArchiveFileSystem(IStreamReader::Ptr stream, [[maybe_unused]] std::string basePath)
    {
        MY_DEBUG_ASSERT(basePath.empty(), "Archive base/inner path is not supported yet");
        return rtti::createInstance<ZipArchiveFileSystem>(std::move(stream));
    }
}  // namespace my::io

#endif
