
// #my_engine_source_file

#pragma once

//#include "my/io/async_stream.h"
#include "my/rtti/type_info.h"
#include "my/debug/dap/stack_trace_provider.h"
//#include "my/serialization/runtime_value.h"

//#include <memory>


namespace my::dap {

// async::Task<Ptr<ReadonlyDictionary>> readDapMessage(io::AsyncStreamPtr stream);

enum class ContinueExecutionMode
{
    Continue,
    Step,
    StepIn,
    StepOut,
    Paused
};


struct MY_ABSTRACT_TYPE IDebugSession
{
    MY_TYPEID(my::dap::IDebugSession)

    virtual ~IDebugSession() = default;


    virtual ContinueExecutionMode Pause(dap::StoppedEventBody ev, StackTraceProviderPtr stackProvider) = 0;

};

//using DebugSessionControlPtr = std::unique_ptr<IDebugSessionControl>;

}  // namespace my::dap
