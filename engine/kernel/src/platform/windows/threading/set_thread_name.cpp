// #my_engine_source_file
#include "my/threading/set_thread_name.h"
// TODO Tracy #include "tracy/Tracy.hpp"

namespace my::threading
{
#if 1//MY_PLATFORM_WINDOWS
    namespace
    {
        constexpr DWORD MS_VC_EXCEPTION = 0x406D1388;

        // clang-format off
        #pragma pack(push, 8)
        typedef struct tagTHREADNAME_INFO
        {
            DWORD dwType;      // Must be 0x1000.
            LPCSTR szName;     // Pointer to name (in user addr space).
            DWORD dwThreadID;  // Thread ID (-1=caller thread).
            DWORD dwFlags;     // Reserved for future use, must be zero.
        } THREADNAME_INFO;
        #pragma pack(pop)
        // clang-format on
    }  // namespace
#endif

    void setThisThreadName([[maybe_unused]] const std::string& name)
    {
#if 1// MY_PLATFORM_WINDOWS
        // https://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx

        THREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = name.c_str();
        info.dwThreadID = std::numeric_limits<DWORD>::max();
        info.dwFlags = 0;
    #pragma warning(push)
    #pragma warning(disable : 6320 6322)
        __try
        {
            ::RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(DWORD), (ULONG_PTR*)&info);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
        }
    #pragma warning(pop)

// TODO Tracy        tracy::SetThreadName(name.c_str());
#endif // MY_PLATFORM_WINDOWS
    }

}  // namespace my::threading
