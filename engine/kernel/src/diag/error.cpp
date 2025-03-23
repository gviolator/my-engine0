// #my_engine_source_header

#include "my/diag/error.h"

namespace my
{
    std::string Error::getDiagMessage() const
    {
        const auto source = getSource();
        if (!source)
        {
            return getMessage();
        }

        const std::string message = source.line ? std::format("{}({}): error:{}", source.filePath, *source.line, getMessage()) : std::format("{}: error:{}", source.filePath, getMessage());

        return {message.data(), message.size()};
    }

}  // namespace my
