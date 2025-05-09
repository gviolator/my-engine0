// #my_engine_source_file

#include "my/io/virtual_file_system.h"
#include "my/rtti/rtti_impl.h"
#include "my/threading/spin_lock.h"


namespace my::io
{
    class VirtualFileSystemImpl final : public IVirtualFileSystem
    {
        MY_REFCOUNTED_CLASS(my::io::VirtualFileSystemImpl, IVirtualFileSystem)
        
    private:
        struct FileSystemEntry
        {
            FileSystemPtr fs;
            unsigned priority;
        };

    public:
        class FsNode
        {
        public:
            FsNode(std::string name): m_name(std::move(name))
            {}

            FsNode(const FsNode&) = delete;

            std::string_view getName() const;

            Result<FsNode*> getChild(std::string_view name);

            FsNode* findChild(std::string_view name);

            FsNode* getNextChild(const FsNode* current = nullptr);

            FileSystemPtr getNextMountedFs(FileSystem* current = nullptr);

            Result<> mount(FileSystemPtr&&, unsigned priority);

            void unmount(const FileSystemPtr&);

            std::vector<FileSystemEntry> getMountedFs();

            bool hasMounts();

        private:
            
            decltype(auto) findChildIter(std::string_view name)
            {
                return std::find_if(m_children.begin(), m_children.end(), [name](const FsNode& c)
                    {
                        return c.m_name == name;
                    });
            }


            const std::string m_name;
            std::list<FsNode> m_children;
            std::vector<FileSystemEntry> m_mountedFs;
            threading::SpinLock m_mutex;
        };

        VirtualFileSystemImpl();

        bool isReadOnly() const override;

        bool exists(const FsPath&, std::optional<FsEntryKind>) override;

        size_t getLastWriteTime(const FsPath&) override;

        IFile::Ptr openFile(const FsPath&, AccessModeFlag accessMode, OpenFileMode openMode) override;

        OpenDirResult openDirIterator(const FsPath& path) override;

        void closeDirIterator(void*) override;

        FsEntry incrementDirIterator(void*) override;

        Result<> createDirectory(const FsPath&) override;

        Result<> remove(const FsPath&, bool recursive = false) override;

        Result<> mount(const FsPath&, FileSystemPtr, unsigned priority) override;

        void unmount(FileSystemPtr) override;

        std::filesystem::path resolveToNativePath(const FsPath& path) override;

    private:

        std::tuple<FsPath, FsNode*> findFsNodeForPath(const FsPath& path);
        FsNode m_root;
    };
}  // namespace my::io
