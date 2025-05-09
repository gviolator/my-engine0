// #my_engine_source_file

#pragma once
#include "my/io/file_system.h"
#include "my/rtti/rtti_impl.h"

namespace my::io
{
    class WinNativeFileSystem final : public IMutableFileSystem,
                                      public INativeFileSystem
    {
        MY_REFCOUNTED_CLASS(my::io::WinNativeFileSystem, IMutableFileSystem, INativeFileSystem)

    public:
        WinNativeFileSystem(std::filesystem::path&& basePath, bool isReadonly);

        bool isReadOnly() const override;

        bool exists(const FsPath&, std::optional<FsEntryKind> kind) override;

        size_t getLastWriteTime(const FsPath&) override;

        IFile::Ptr openFile(const FsPath&, AccessModeFlag accessMode, OpenFileMode openMode) override;

        OpenDirResult openDirIterator(const FsPath& path) override;

        void closeDirIterator(void*) override;

        FsEntry incrementDirIterator(void*) override;

        Result<> createDirectory(const FsPath&) override;

        Result<> remove(const FsPath&, bool recursive = false) override;

        std::filesystem::path resolveToNativePath(const FsPath& path) override;

    private:

        std::filesystem::path resolveToNativePathNoCheck(const FsPath& path);

        const std::filesystem::path m_basePath;
        const bool m_isReadOnly;
    };
}