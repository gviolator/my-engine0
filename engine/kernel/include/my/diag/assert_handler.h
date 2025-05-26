// #my_engine_source_file

#pragma once

#include "my/diag/assert.h"
#include "my/diag/source_info.h"
#include "my/kernel/kernel_config.h"

namespace my::diag
{
    struct FailureData
    {
        /**
         * @brief information about problem.
         * @param condition - possible condition triggered fatal.
         * @param file - file where was triggered fatal.
         * @param line - line where was triggered fatal.
         * @param function - function where was triggered fatal.
         * @param message - additional info about problem.
         */
        const AssertionKind kind = AssertionKind::Default;
        const SourceInfo source;
        const std::string_view condition;
        const std::string_view message;

        FailureData(AssertionKind inKind, SourceInfo inSource, std::string_view inCondition, std::string_view inMessage) :
            kind(inKind),
            source(inSource),
            condition(inCondition),
            message(inMessage)
        {
        }

        FailureData(const FailureData&) = delete;
        FailureData& operator=(const FailureData&) = delete;
    };

    struct IAssertHandler
    {
        virtual ~IAssertHandler() = default;

        virtual FailureActionFlag handleAssertFailure(const FailureData&) = 0;
    };

    using AssertHandlerPtr = std::unique_ptr<IAssertHandler>;

    MY_KERNEL_EXPORT void setAssertHandler(AssertHandlerPtr newHandler, AssertHandlerPtr* prevHandler = nullptr);

    MY_KERNEL_EXPORT IAssertHandler* getCurrentAssertHandler();

    MY_KERNEL_EXPORT AssertHandlerPtr createDefaultAssertHandler();
}  // namespace my::diag
