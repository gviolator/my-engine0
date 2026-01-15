// #my_engine_source_file
#pragma once

#include "my/io/stream.h"
#include "my/kernel/kernel_config.h"

#include <limits>
#include <memory>
#include <span>
#include <string_view>
#include <utility>

namespace my::debug {

class SourceMap
{
public:
    static inline constexpr unsigned NotIndex = std::numeric_limits<unsigned>::max();

    struct LineInfo
    {
        std::pair<unsigned, unsigned> compiledPos;
        std::pair<unsigned, unsigned> sourcePos;

        unsigned sourceIndex = NotIndex;
        std::string_view name;
    };

    virtual ~SourceMap() = default;

    virtual void SetFirstLineNo(unsigned lineNo) = 0;

    virtual unsigned GetFirstLineNo() const = 0;

    virtual unsigned GetCompiledLinesCount() const = 0;

    virtual std::span<const LineInfo> GetCompiledLine(unsigned compiledLineNo) const = 0;

    virtual unsigned MapSourceLine(unsigned sourceLine) const = 0;

    virtual std::string_view GetSourceName(unsigned sourceIndex) const = 0;

    virtual unsigned GetSourcesCount() const = 0;
};

using SourceMapPtr = std::shared_ptr<SourceMap>;

MY_KERNEL_EXPORT Result<SourceMapPtr> ParseSourceMap(io::IStream&);

}  // namespace my::debug
