// #my_engine_source_file
#pragma once

#include "my/async/task.h"
#include "my/debug/dap/dap.h"
#include "my/debug/dap/debug_session.h"
#include "my/debug/dap/debugee_control.h"
#include "my/kernel/kernel_config.h"
#include "my/network/address.h"
#include "my/rtti/ptr.h"
#include "my/rtti/type_info.h"
#include "my/runtime/async_disposable.h"

#include <memory>
#include <tuple>
#include <vector>

namespace my::dap {

using StartDebugSessionResult = std::tuple<Capabilities, DebuggeeControlPtr>;

/**
 */
struct MY_ABSTRACT_TYPE IDebuggerHost
{
    MY_TYPEID(my::dap::IDebuggerHost)

    struct DebugLocation
    {
        std::string id;
        std::string description;

        MY_CLASS_FIELDS(
            CLASS_FIELD(id),
            CLASS_FIELD(description))
    };

    virtual ~IDebuggerHost() = default;

    virtual std::vector<DebugLocation> getDebugLocations() = 0;

    virtual async::Task<StartDebugSessionResult> startDebugSession(std::string locationId, const dap::InitializeRequestArguments& initArgs, IDebugSession& session) = 0;
};

MY_KERNEL_EXPORT async::Task<Ptr<IAsyncDisposable>> runDebuggerHost(network::Address listenAddress, std::unique_ptr<IDebuggerHost>);

// MY_KERNEL_EXPORT

}  // namespace my::dap
