// #my_engine_source_header

#pragma once

#include "my/diag/check.h"
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

    struct ICheckHandler
    {
        virtual ~ICheckHandler() = default;

        virtual FailureActionFlag handleCheckFailure(const FailureData&) = 0;
    };

    using CheckHandlerPtr = std::unique_ptr<ICheckHandler>;

    MY_KERNEL_EXPORT void setCheckHandler(CheckHandlerPtr newHandler, CheckHandlerPtr* prevHandler = nullptr);

    MY_KERNEL_EXPORT ICheckHandler* getCurrentCheckHandler();

    MY_KERNEL_EXPORT CheckHandlerPtr createDefaultCheckHandler();
}  // namespace my::diag
