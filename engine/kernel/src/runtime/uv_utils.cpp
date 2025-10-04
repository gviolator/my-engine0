// #my_engine_source_file
#include "runtime/uv_utils.h"

namespace my {

std::string_view getUVErrorMessage(int code)
{
    if (code == 0)
    {
        return {};
    }

    const char* const message = uv_strerror(code);
    if (message == nullptr)
    {
        const char* const UnknownErrorMessage = "Unknown error code";
        return UnknownErrorMessage;
    }
    return message;
}

}  // namespace my
