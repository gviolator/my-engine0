// #my_engine_source_file
#include "in_mem_filesystem.h"

#include "my/io/memory_stream.h"
#include "my/memory/buffer.h"

namespace my::io
{
    namespace
    {
        class InMemoryFile final : public io::IFile,
                                   public io_detail::IFileInternal
        {
            MY_REFCOUNTED_CLASS(my::io::InMemoryFile, io::IFile, io_detail::IFileInternal)

        public:
            InMemoryFile(ReadOnlyBuffer buffer) :
                m_buffer(std::move(buffer))
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

            io::StreamBasePtr createStream(std::optional<io::AccessModeFlag> access) override
            {
                return io::createReadonlyMemoryStream({m_buffer.data(), m_buffer.size()});
            }

            io::AccessModeFlag getAccessMode() const override
            {
                return io::AccessMode::Read;
            }

            size_t getSize() const override
            {
                return m_buffer.size();
            }

            io::FsPath getPath() const override
            {
                return m_vfsPath;
            }

            void setVfsPath(io::FsPath path) override
            {
                m_vfsPath = std::move(path);
            }

        private:
            io::FsPath m_vfsPath;
            ReadOnlyBuffer m_buffer;
        };
    }

    bool InMemoryFileSystem::isReadOnly() const
    {
        return true;
    }

    bool InMemoryFileSystem::exists(const FsPath& path, std::optional<FsEntryKind> kind)
    {
        if (kind && *kind == FsEntryKind::Directory)
        {
            return false;
        }

        return m_content.contains(path);
    }

    size_t InMemoryFileSystem::getLastWriteTime(const FsPath&)
    {
        return 0;
    }

    FilePtr InMemoryFileSystem::openFile(const FsPath& path, AccessModeFlag accessMode, OpenFileMode openMode)
    {
        auto iter = m_content.find(path);
        if (iter == m_content.end())
        {
            return nullptr;
        }

        return rtti::createInstance<InMemoryFile>(iter->second);
    }

    FileSystem::OpenDirResult InMemoryFileSystem::openDirIterator(const FsPath& path)
    {
        return MakeError("Not supported");
    }

    void InMemoryFileSystem::closeDirIterator(void*)
    {
        MY_FAILURE("InMemoryFileSystem::closeDirIterator not implemented");
    }

    FsEntry InMemoryFileSystem::incrementDirIterator(void*)
    {
        MY_FAILURE("InMemoryFileSystem::incrementDirIterator not implemented");
        return {};
    }

    void InMemoryFileSystem::addContent(FsPath path, ReadOnlyBuffer buffer)
    {
        m_content.emplace(std::move(path), std::move(buffer));
    }



}  // namespace my::io