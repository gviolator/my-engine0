// #my_engine_source_file
#include "shared_library.h"

#include "my/diag/assert.h"
#include "my/utils/string_conv.h"

namespace my::os
{
    Result<SharedLibrary> SharedLibrary::open(const std::filesystem::path& dlPath)
    {
#ifdef _WIN32
        static_assert(std::is_same_v<std::filesystem::path::value_type, wchar_t>);
        const std::string utf8Path = strings::wstringToUtf8(dlPath.wstring());
#else
        static_assert(std::is_same_v<std::filesystem::path::value_type, char>);
        const std::string utf8Path = strings::wstringToUtf8(dlPath.string());
#endif
        uv_lib_t handle;
        const int dlOpenResult = uv_dlopen(utf8Path.c_str(), &handle);
        if (dlOpenResult != 0)
        {
            const char* dlErrorMessage = uv_dlerror(&handle);
            return MakeError(dlErrorMessage);
        }

        return SharedLibrary{std::move(handle)};
    }

    SharedLibrary::SharedLibrary(const uv_lib_t& handle) :
        m_handle(handle)
    {
    }

    SharedLibrary::SharedLibrary(SharedLibrary&& other) :
        m_handle(other.m_handle)
    {
        other.m_handle.reset();
    }

    SharedLibrary::~SharedLibrary()
    {
        close();
    }

    SharedLibrary& SharedLibrary::operator=(SharedLibrary&& other)
    {
        MY_DEBUG_ASSERT(!m_handle, "re-assign to already loaded shared library is prohibited");
        close();
        
        m_handle = other.m_handle;
        other.m_handle.reset();

        return *this;
    }    

    SharedLibrary::operator bool() const
    {
        return static_cast<bool>(m_handle);
    }

    void SharedLibrary::close()
    {
        if (m_handle)
        {
            uv_dlclose(&m_handle.value());
            m_handle.reset();
        }
    }
}  // namespace my::os
