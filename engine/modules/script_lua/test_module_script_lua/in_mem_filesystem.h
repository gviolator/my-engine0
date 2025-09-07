// #my_engine_source_file
#pragma once
#include "my/io/file_system.h"
#include "my/rtti/rtti_impl.h"
#include "my/memory/buffer.h"

namespace my::io
{
    class InMemoryFileSystem final : public FileSystem
    {
        MY_REFCOUNTED_CLASS(my::io::InMemoryFileSystem, FileSystem)

    public:
        InMemoryFileSystem() = default;

        bool isReadOnly() const override;

        bool exists(const FsPath&, std::optional<FsEntryKind> kind = std::nullopt) override;

        size_t getLastWriteTime(const FsPath&) override;
        
        FilePtr openFile(const FsPath&, AccessModeFlag accessMode, OpenFileMode openMode) override;

        OpenDirResult openDirIterator(const FsPath& path) override;

        void closeDirIterator(void*) override;

        FsEntry incrementDirIterator(void*) override;

        void addContent(FsPath path, ReadOnlyBuffer buffer);
    private:
        std::unordered_map<io::FsPath, ReadOnlyBuffer> m_content;
    };
}