// #my_engine_source_file

#include "win_file.h"

#include "my/platform/windows/diag/win_error.h"
#include "my/threading/lock_guard.h"

namespace fs = std::filesystem;

namespace my::io
{
    namespace
    {
        DWORD get_file_access(AccessModeFlag accessMode)
        {
            DWORD accessFlag = 0;
            if (accessMode && AccessMode::Read)
            {
                accessFlag |= GENERIC_READ;
            }
            else if (accessMode && AccessMode::Write)
            {
                accessFlag |= GENERIC_WRITE;
            }

            return accessFlag;
        }

        HANDLE create_file(const fs::path& path, AccessModeFlag accessMode, OpenFileMode openMode, DWORD attributes = FILE_ATTRIBUTE_NORMAL)
        {
            DWORD accessFlags = 0;
            if (accessMode && AccessMode::Read)
            {
                accessFlags |= GENERIC_READ;
            }
            else if (accessMode && AccessMode::Write)
            {
                accessFlags |= GENERIC_WRITE;
            }

            const DWORD shareFlags = FILE_SHARE_READ;
            const DWORD createFlag = EXPR_Block
            {
                if (openMode == OpenFileMode::CreateAlways)
                {
                    return CREATE_ALWAYS;
                }
                else if (openMode == OpenFileMode::CreateNew)
                {
                    return CREATE_NEW;
                }
                else if (openMode == OpenFileMode::OpenAlways)
                {
                    return OPEN_ALWAYS;
                }
                else if (openMode == OpenFileMode::OpenExisting)
                {
                    return OPEN_EXISTING;
                }

                MY_FAILURE("Unknown openMode");
                return 0;
            };

            const std::wstring wcsPath = path.wstring();
            const HANDLE fileHandle = ::CreateFileW(wcsPath.c_str(), accessFlags, shareFlags, nullptr, createFlag, attributes, nullptr);

            return fileHandle;
        }
    }  // namespace
    /*
    class WinFileMapping final : public virtual IMemoryMappedObject
    {
        MY_REFCOUNTED_CLASS(my::io::WinFileMapping, IMemoryMappedObject)

    public:
        WinFileMapping(my::Ptr<WinFile> file) :
            m_file(std::move(file))
        {
            MY_DEBUG_ASSERT(m_file);
            MY_DEBUG_ASSERT(m_file->isOpened());
            if(!m_file || !m_file->isOpened())
            {
                return;
            }

            const DWORD pageProtectFlag = m_file->getAccessMode() && AccessMode::Write ? PAGE_READWRITE : PAGE_READONLY;
            m_fileMappingHandle = ::CreateFileMappingA(m_file->getFileHandle(), nullptr, pageProtectFlag, 0, 0, nullptr);

            MY_DEBUG_ASSERT(m_fileMappingHandle != nullptr);
        }

        ~WinFileMapping()
        {
            if(m_fileMappingHandle != nullptr)
            {
                CloseHandle(m_fileMappingHandle);
            }
        }

        bool isValid() const
        {
            return m_fileMappingHandle != nullptr;
        }

        void* memMap(size_t offset, std::optional<size_t> count) override
        {
            MY_DEBUG_ASSERT(m_fileMappingHandle != nullptr);
            MY_DEBUG_ASSERT(m_file);
            MY_DEBUG_ASSERT(m_file->getAccessMode().hasAny(AccessMode::Read, AccessMode::Write));
            MY_DEBUG_ASSERT(offset < m_file->getSize());

            const DWORD access = (m_file->getAccessMode() && AccessMode::Write) ? (FILE_MAP_READ | FILE_MAP_WRITE) : FILE_MAP_READ;

            const DWORD offsetHigh = static_cast<DWORD>((offset & 0xFFFFFFFF00000000LL) >> 32);
            const DWORD offsetLow = static_cast<DWORD>(offset & 0xFFFFFFFFLL);
            const SIZE_T mapSize = static_cast<SIZE_T>(count.value_or(0));

            void* const ptr = ::MapViewOfFile(m_fileMappingHandle, access, offsetHigh, offsetLow, mapSize);
            MY_DEBUG_ASSERT(ptr, "MapViewOfFile returns nullptr");

            return ptr;
        }

        void memUnmap(const void* ptr) override
        {
            MY_DEBUG_ASSERT(m_fileMappingHandle != nullptr);

            if(ptr != nullptr)
            {
                [[maybe_unused]]
                const BOOL unmapSuccess = ::UnmapViewOfFile(ptr);
                MY_DEBUG_ASSERT(unmapSuccess);
            }
        }

    private:
        const my::Ptr<WinFile> m_file;
        HANDLE m_fileMappingHandle = nullptr;
    };
*/
    WinFile::WinFile(const fs::path& path, AccessModeFlag accessMode, OpenFileMode openMode, DWORD attributes) :
        m_accessMode(accessMode),
        m_fileHandle(create_file(path, accessMode, openMode, attributes))
    {
        // MY_DEBUG_ASSERT(m_fileHandle != INVALID_HANDLE_VALUE, "Fail to open file: ({})", path);
    }

    WinFile::~WinFile()
    {
        if (m_fileHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_fileHandle);
        }
    }

    bool WinFile::supports(FileFeature feature) const
    {
        if (feature == FileFeature::AsyncStreaming)
        {
            return false;
        }

        if (feature == FileFeature::MemoryMapping)
        {
            return true;
        }

        return false;
    }

    bool WinFile::isOpened() const
    {
        return m_fileHandle != INVALID_HANDLE_VALUE;
    }

    /*
        IMemoryMappedObject::Ptr WinFile::getMemoryMappedObject()
        {
            if(auto mappedObject = rtti::createInstance<WinFileMapping>(rtti::Acquire{this}); mappedObject->isValid())
            {
                return mappedObject;
            }

            return nullptr;
        }*/

    StreamBasePtr WinFile::createStream([[maybe_unused]] std::optional<AccessModeFlag> accessMode)
    {
        MY_DEBUG_ASSERT(isOpened());
        if (!isOpened())
        {
            return nullptr;
        }

        std::array<wchar_t, 1024> path;

        // TODO: use dynamic buffer
        [[maybe_unused]]
        const DWORD pathLen = ::GetFinalPathNameByHandleW(m_fileHandle, path.data(), static_cast<DWORD>(path.size()), FILE_NAME_NORMALIZED);
        MY_DEBUG_ASSERT(::GetLastError() == 0);

        const std::wstring_view pathStr{path.data(), static_cast<size_t>(pathLen)};

        return createNativeFileStream(pathStr, m_accessMode, OpenFileMode::OpenExisting);
    }

    void* WinFile::memMap([[maybe_unused]] size_t offset, [[maybe_unused]] size_t count)
    {
        MY_DEBUG_ASSERT(isOpened());
        MY_DEBUG_ASSERT(getAccessMode().anyIsSet(AccessMode::Read, AccessMode::Write));
        MY_DEBUG_ASSERT(offset < getSize());

        lock_(m_mutex);
        if (++m_fileMappingCounter == 1)
        {
            const DWORD access = (getAccessMode() && AccessMode::Write) ? (FILE_MAP_READ | FILE_MAP_WRITE) : FILE_MAP_READ;

            const DWORD offsetHigh = static_cast<DWORD>((offset & 0xFFFFFFFF00000000LL) >> 32);
            const DWORD offsetLow = static_cast<DWORD>(offset & 0xFFFFFFFFLL);
            const SIZE_T mapSize = static_cast<SIZE_T>(count);

            m_mappedPtr = ::MapViewOfFile(m_fileMappingHandle, access, offsetHigh, offsetLow, mapSize);
            MY_DEBUG_ASSERT(m_mappedPtr, "MapViewOfFile returns nullptr");
        }

        return m_mappedPtr;
    }

    void WinFile::memUnmap(const void* ptr)
    {
        MY_DEBUG_ASSERT(ptr == nullptr || ptr == m_mappedPtr);

        const void* const ptrToUnmap = EXPR_Block->const void*
        {
            lock_(m_mutex);
            MY_DEBUG_ASSERT(m_fileMappingCounter > 0);
            if (m_fileMappingCounter == 0 || --m_fileMappingCounter > 0)
            {
                return nullptr;
            }

            return m_mappedPtr;
        };

        if (ptrToUnmap)
        {
            const BOOL unmapSuccess = ::UnmapViewOfFile(ptrToUnmap);
            MY_DEBUG_ASSERT(unmapSuccess);
        }
    }

    size_t WinFile::getSize() const
    {
        MY_DEBUG_ASSERT(isOpened());

        if (!isOpened())
        {
            return 0;
        }

        LARGE_INTEGER size{};
        if (::GetFileSizeEx(m_fileHandle, &size) != TRUE)
        {
            return 0;
        }

        return static_cast<size_t>(size.QuadPart);
    }

    FsPath WinFile::getPath() const
    {
        return m_vfsPath;
    }

    void WinFile::setVfsPath(io::FsPath path)
    {
        m_vfsPath = std::move(path);
    }

    fs::path WinFile::getNativePath() const
    {
        MY_DEBUG_ASSERT(isOpened());
        if (!isOpened())
        {
            return {};
        }

        std::array<char, 1024> path;

        [[maybe_unused]]
        const DWORD pathLen = ::GetFinalPathNameByHandleA(m_fileHandle, path.data(), static_cast<DWORD>(path.size()), FILE_NAME_NORMALIZED);
        MY_DEBUG_ASSERT(::GetLastError() == 0);

        return std::string{path.data(), pathLen};
    }

    AccessModeFlag WinFile::getAccessMode() const
    {
        return m_accessMode;
    }

    // WinFileStream::WinFileStream(HANDLE fileHandle) :
    //     m_fileHandle(fileHandle)
    // {
    //     MY_DEBUG_ASSERT(m_fileHandle != INVALID_HANDLE_VALUE);
    // }

    WinFileStream::WinFileStream(const fs::path& path, AccessModeFlag accessMode, OpenFileMode openMode) :
        m_fileHandle(create_file(path, accessMode, openMode)),
        m_accessMode(accessMode)
    {
    }

    WinFileStream::~WinFileStream()
    {
        if (m_fileHandle != INVALID_HANDLE_VALUE)
        {
            ::CloseHandle(m_fileHandle);
        }
    }

    size_t WinFileStream::getPosition() const
    {
        MY_DEBUG_ASSERT(m_fileHandle != INVALID_HANDLE_VALUE);
        if (m_fileHandle == INVALID_HANDLE_VALUE)
        {
            return 0;
        }

        LARGE_INTEGER offset{.QuadPart = 0};
        LARGE_INTEGER currentOffset{.QuadPart = 0};

        [[maybe_unused]]
        const auto success = ::SetFilePointerEx(getFileHandle(), offset, &currentOffset, FILE_CURRENT);
        MY_DEBUG_ASSERT(success);

        return static_cast<size_t>(currentOffset.QuadPart);
    }

    size_t WinFileStream::setPosition(OffsetOrigin origin, int64_t value)
    {
        MY_DEBUG_ASSERT(m_fileHandle != INVALID_HANDLE_VALUE);
        if (m_fileHandle == INVALID_HANDLE_VALUE)
        {
            return 0;
        }

        const LARGE_INTEGER offset{.QuadPart = static_cast<LONGLONG>(value)};
        LARGE_INTEGER newOffset{.QuadPart = 0};

        const DWORD offsetMethod = EXPR_Block->DWORD
        {
            if (origin == OffsetOrigin::Begin)
            {
                return FILE_BEGIN;
            }
            else if (origin == OffsetOrigin::End)
            {
                return FILE_END;
            }
            MY_DEBUG_ASSERT(origin == OffsetOrigin::Current);

            return FILE_CURRENT;
        };

        [[maybe_unused]]
        const auto success = ::SetFilePointerEx(getFileHandle(), offset, &newOffset, offsetMethod);
        MY_DEBUG_ASSERT(success);

        return static_cast<size_t>(newOffset.QuadPart);
    }

    Result<size_t> WinFileStream::read(std::byte* ptr, size_t count)
    {
        MY_DEBUG_ASSERT(isOpened());
        if (!isOpened())
        {
            return MakeError("File is not opened");
        }

        DWORD actualReadCount = 0;

        const bool readOk = ::ReadFile(getFileHandle(), reinterpret_cast<void*>(ptr), static_cast<DWORD>(count), &actualReadCount, nullptr);
        if (!readOk)
        {
            return MakeErrorT(diag::WinCodeError)("Fail to read file");
        }

        return static_cast<size_t>(actualReadCount);
    }

    Result<size_t> WinFileStream::write(const std::byte* ptr, size_t count)
    {
        MY_DEBUG_ASSERT(isOpened());
        if (!isOpened())
        {
            return MakeError("File is not opened");
        }

        DWORD actualWriteCount = 0;

        const bool writeOk = ::WriteFile(getFileHandle(), reinterpret_cast<const void*>(ptr), static_cast<DWORD>(count), &actualWriteCount, nullptr);
        if (!writeOk)
        {
            return MakeErrorT(diag::WinCodeError)("Fail to write file");
        }

        return static_cast<size_t>(actualWriteCount);
    }

    void WinFileStream::flush()
    {
        FlushFileBuffers(getFileHandle());
    }

    bool WinFileStream::canSeek() const
    {
        return true;
    }

    bool WinFileStream::canRead() const
    {
        return m_accessMode && AccessMode::Read;
    }

    bool WinFileStream::canWrite() const
    {
        return m_accessMode && AccessMode::Write;
    }

    StreamBasePtr createNativeFileStream(fs::path path, AccessModeFlag accessMode, OpenFileMode openMode)
    {
        accessMode -= AccessMode::Async;

        // MY_DEBUG_ASSERT(!(accessMode && AccessMode::Async), "Async IO not supported yet");

        return rtti::createInstance<WinFileStream>(path, accessMode, openMode);

        // const eastl::wstring wcsPath = strings::utf8ToWString(path);
        //  const wchar_t* const wcsPath = path.c_str();

        // if (accessMode == AccessMode::Read)
        // {
        //     if (auto stream = rtti::createInstance<WinFileStreamReader>(wcsPath.c_str(), accessMode, openMode); stream->isOpened())
        //     {
        //         return stream;
        //     }
        // }
        // else if (accessMode == AccessMode::Write)
        // {
        //     if (auto stream = rtti::createInstance<WinFileStreamWriter>(wcsPath.c_str(), accessMode, openMode); stream->isOpened())
        //     {
        //         return stream;
        //     }
        // }

        // // const auto attributes = GetFileAttributesA(fullPath.c_str());
        // // if (attributes == INVALID_FILE_ATTRIBUTES)
        // // {
        // //     return nullptr;
        // // }

        // // auto file = rtti::createInstance<WinFile>(fullPath.c_str(), accessMode, openMode);
        // // return file->isOpened() ? file : nullptr;

        // return nullptr;
    }

}  // namespace my::io
