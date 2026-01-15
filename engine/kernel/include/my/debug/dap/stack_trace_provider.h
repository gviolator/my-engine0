// #my_engine_source_file
#pragma once

#include "my/debug/dap/dap.h"
#include "my/rtti/type_info.h"

#include <memory>

namespace my::dap {

/**
    Provides stack related information.
    All methods deal with stack frame id (they can not belongs to the IDebuggee interface)
 */
struct MY_ABSTRACT_TYPE IStackTraceProvider
{
    MY_TYPEID(my::dap::IStackTraceProvider)

    virtual ~IStackTraceProvider() = default;

    virtual StackTraceResponseBody GetStackTrace(const StackTraceArguments&) = 0;

    virtual std::vector<dap::Scope> GetScopes(unsigned stackFrameId) = 0;

    virtual Result<dap::EvaluateResponseBody> EvaluateWatch(const dap::EvaluateArguments&) = 0;
};

using StackTraceProviderPtr = std::unique_ptr<IStackTraceProvider>;

}  // namespace my::dap
