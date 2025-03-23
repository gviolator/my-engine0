// #my_engine_source_header

#pragma once
#include "my/diag/error.h"

namespace my
{
    /**
     */
    class OperationCancelledError : public DefaultError<>
    {
        MY_ERROR(my::OperationCancelledError, DefaultError<>)
    public:
        OperationCancelledError(const diag::SourceInfo& sourceInfo, std::string_view description = "Operation was cancelled") :
            DefaultError<>(sourceInfo, std::string{description})
        {
        }
    };
}  // namespace my
