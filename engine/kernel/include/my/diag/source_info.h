// #my_engine_source_file

#pragma once

#include <optional>
#include <string_view>

#include "my/kernel/kernel_config.h"
#include "my/utils/preprocessor.h"

namespace my::diag
{

    /**
     */
    struct MY_KERNEL_EXPORT SourceInfo
    {
        std::string_view moduleName;
        std::string_view functionName;
        std::string_view filePath;
        std::optional<unsigned> line;

        SourceInfo() = default;
        SourceInfo(const SourceInfo&) = default;

        SourceInfo(std::string_view module, std::string_view func, std::string_view file, std::optional<unsigned> ln = std::nullopt) :
            moduleName(module),
            functionName(func),
            filePath(file),
            line(ln)
        {
        }

        SourceInfo(std::string_view func, std::string_view file, std::optional<unsigned> ln = std::nullopt) :
            SourceInfo({}, func, file, ln)
        {
        }

        SourceInfo& operator=(const SourceInfo&) = default;

        explicit operator bool() const
        {
            return !functionName.empty() || !filePath.empty();
        }
    };

}  // namespace my::diag

// clang-format off
#define MY_INLINED_SOURCE_INFO ::my::diag::SourceInfo { std::string_view{__FUNCTION__}, std::string_view{__FILE__}, static_cast<unsigned>(__LINE__) }
// clang-format on
