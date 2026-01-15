// #my_engine_source_file
#pragma once
#include "my/async/executor.h"
#include "my/async/task_base.h"
#include "my/debug/dap/dap.h"
#include "my/serialization/runtime_value.h"
#include "my/utils/result.h"

#include <memory>
#include <vector>

namespace my::dap {

struct MY_ABSTRACT_TYPE IDebuggeeControl
{
    MY_TYPEID(my::dap::IDebuggeeControl)

    virtual ~IDebuggeeControl() = default;

    virtual async::ExecutorPtr getExecutor() = 0;

    virtual async::Task<> disconnect() = 0;

    //virtual Capabilities initializeSession(const InitializeRequestArguments& initArgs) = 0;

    virtual Result<> configureLaunch(RuntimeValuePtr configuration) = 0;

    virtual Result<> configureAttach(RuntimeValuePtr configuration) = 0;

    virtual Result<> configurationDone() = 0;

    virtual void requestPause() = 0;

    virtual Result<std::vector<dap::Breakpoint>> setBreakpoints(const dap::SetBreakpointsArguments&) = 0;

    virtual Result<std::vector<dap::Breakpoint>> setFunctionBreakpoints(const dap::SetFunctionBreakpointsArguments&) = 0;

    virtual Result<std::vector<dap::Thread>> getThreads() = 0;

    virtual Result<std::vector<dap::Variable>> getVariables(const dap::VariablesArguments&) = 0;

    virtual Result<dap::EvaluateResponseBody> evaluate(const dap::EvaluateArguments&) = 0;
};

using DebuggeeControlPtr = std::unique_ptr<IDebuggeeControl>;

}  // namespace my::dap
