// #my_engine_source_file
#include "runtime/uv_utils.h"

namespace my
{
    std::string getUVErrorMessage(int code)
    {
        if (code == 0)
        {
            return {};
        }

        const char* const message = uv_strerror(code);
        if (message == nullptr)
        {
            return "Unknown error code: " + std::to_string(code);
        }
        return message;
    }
}  // namespace my
