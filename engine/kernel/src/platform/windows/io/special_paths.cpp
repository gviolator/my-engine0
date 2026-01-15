// #my_engine_source_file

#include "my/io/special_paths.h"

#include <ShlObj.h>
#include <tchar.h>

#include "my/diag/assert.h"
#include "my/diag/logging.h"
#include "my/memory/runtime_stack.h"
#include "my/platform/windows/diag/win_error.h"
#include "my/utils/string_conv.h"
#include "my/threading/lock_guard.h"

namespace fs = std::filesystem;

namespace my::io
{
    namespace
    {
        /**
            see: https://learn.microsoft.com/en-us/windows/win32/api/objbase/nf-objbase-coinitialize
         */
        struct CoInitializeThreadGuard
        {
            CoInitializeThreadGuard()
            {
                const HRESULT coInitRes = ::CoInitialize(nullptr);
                MY_ASSERT(coInitRes == S_OK || coInitRes == S_FALSE);
            }

            ~CoInitializeThreadGuard()
            {
                ::CoUninitialize();
            }
        };

        std::optional<KNOWNFOLDERID> knownFolderEnum2Id(KnownFolder folder)
        {
            if (folder == KnownFolder::UserDocuments)
            {
                return FOLDERID_Documents;
            }
            else if (folder == KnownFolder::LocalAppData)
            {
                return FOLDERID_LocalAppData;
            }
            else if (folder == KnownFolder::UserHome)
            {
                return FOLDERID_Profile;
            }

            return std::nullopt;
        }

        fs::path getKnownFolderPathById(KNOWNFOLDERID folderId)
        {
            // CO must be initialized per thread.
            static thread_local const CoInitializeThreadGuard coInitGuard;

            IKnownFolderManager* folderManager = nullptr;
            IKnownFolder* knownFolder = nullptr;
            wchar_t* folderPathBuffer = nullptr;

            scope_on_leave
            {
                if (folderManager)
                {
                    folderManager->Release();
                }

                if (knownFolder)
                {
                    knownFolder->Release();
                }

                if (folderPathBuffer)
                {
                    ::CoTaskMemFree(folderPathBuffer);
                }
            };

            HRESULT hr = ::CoCreateInstance(CLSID_KnownFolderManager, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&folderManager));
            if (FAILED(hr))
            {
                return {};
            }

            hr = folderManager->GetFolder(folderId, &knownFolder);
            if (FAILED(hr))
            {
                return {};
            }

            hr = knownFolder->GetPath(KF_FLAG_DEFAULT, &folderPathBuffer);
            if (FAILED(hr))
            {
                return {};
            }

            const size_t len = wcslen(folderPathBuffer);
            return std::wstring_view{folderPathBuffer, len};
        }

        


        std::filesystem::path getExecutableLocation()
        {
            [[maybe_unused]] constexpr size_t MaxModulePathLen = 2048;

            rtstack_scope;

            StackContainer<std::vector, wchar_t> exeModulePath;

            //StackVector<wchar_t> exeModulePath;
            //std::vector<wchar_t, RtStackStdAllocator<wchar_t>> exeModulePath;

            exeModulePath.resize(MAX_PATH);

            DWORD len = 0;
            DWORD err = 0;

            do
            {
                len = ::GetModuleFileNameW(nullptr, exeModulePath.data(), static_cast<DWORD>(exeModulePath.size()));
                err = ::GetLastError();
                MY_DEBUG_ASSERT(err == 0 || err == ERROR_INSUFFICIENT_BUFFER);

                if (err == ERROR_INSUFFICIENT_BUFFER)
                {
                    MY_FATAL(exeModulePath.size() < MaxModulePathLen);
                    exeModulePath.resize(exeModulePath.size() * 2);
                }
            } while (err != 0);

            std::wstring_view pathStr{exeModulePath.data(), static_cast<size_t>(len)};

            const auto sepPos = pathStr.rfind(std::filesystem::path::preferred_separator);
            MY_FATAL(sepPos != std::wstring_view::npos);

            pathStr.remove_suffix(pathStr.size() - sepPos);


            return pathStr;
        }

    }  // namespace

    std::filesystem::path getNativeTempFilePath(std::string_view prefixFileName)
    {
        rtstack_scope;

        StackContainer<std::vector, wchar_t> tempDirectoryPathBuffer(MAX_PATH);
        StackContainer<std::vector, wchar_t> tempFilePathBuffer(MAX_PATH);

        DWORD tempDirectoryPathLength = ::GetTempPathW(static_cast<DWORD>(tempDirectoryPathBuffer.size()), tempDirectoryPathBuffer.data());
        if (tempDirectoryPathLength > tempDirectoryPathBuffer.size() || (tempDirectoryPathLength == 0))
        {
            MY_FAILURE("GetTempPath failed");
            return {};
        }

        UINT result = ::GetTempFileNameW(tempDirectoryPathBuffer.data(), TEXT(strings::utf8ToWString(prefixFileName).c_str()), 0, tempFilePathBuffer.data());
        if (result == 0)
        {
            MY_FAILURE("GetTempFileName failed");
            return {};
        }

        return strings::wstringToUtf8(std::wstring_view{tempFilePathBuffer.data(), tempFilePathBuffer.size()});
    }

    std::filesystem::path getKnownFolderPath(KnownFolder folder)
    {
        namespace fs = std::filesystem;

        static std::shared_mutex knownFoldersMutex;
        static std::unordered_map<KnownFolder, fs::path> knownFolders;

        {
            const std::shared_lock lock(knownFoldersMutex);
            if (auto folderPath = knownFolders.find(folder); folderPath != knownFolders.end())
            {
                return folderPath->second;
            }
        }

        const std::lock_guard lock(knownFoldersMutex);
        if (auto folderPath = knownFolders.find(folder); folderPath != knownFolders.end())
        {
            return folderPath->second;
        }

        if (folder == KnownFolder::Temp)
        {
            wchar_t tempFolderPath[MAX_PATH];
            const DWORD len = ::GetTempPathW(MAX_PATH, tempFolderPath);
            if (len == 0)
            {
                mylog_error("Fail to get temp path:{}", diag::getWinErrorMessageA(diag::getAndResetLastErrorCode()));
                return {};
            }

            return std::wstring_view{tempFolderPath, static_cast<size_t>(len)};
        }
        else if (folder == KnownFolder::Current)
        {
            return fs::current_path();
        }
        else if (folder == KnownFolder::ExecutableLocation)
        {
            return getExecutableLocation();
        }

        const std::optional<KNOWNFOLDERID> folderId = knownFolderEnum2Id(folder);
        if (!folderId)
        {
            return {};
        }

        fs::path folderPath = getKnownFolderPathById(*folderId);
        if (!folderPath.empty())
        {
            knownFolders[folder] = folderPath;
        }

        return folderPath;
    }
}  // namespace my::io

