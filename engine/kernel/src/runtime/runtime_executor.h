// #my_engine_source_file
#pragma once

#include <list>

#include "my/async/executor.h"
#include "my/rtti/rtti_impl.h"
#include "my/runtime/internal/runtime_component.h"
#include "my/threading/critical_section.h"
#include "runtime/uv_handle.h"


namespace my {

class RuntimeThreadExecutor final : public async::Executor, public IRuntimeComponent
{
    MY_REFCOUNTED_CLASS(my::RuntimeThreadExecutor, async::Executor, IRuntimeComponent)

public:
    RuntimeThreadExecutor();

private:
    void waitAnyActivity() noexcept override;
    void scheduleInvocation(Invocation) noexcept override;
    bool hasWorks() override;


    UvHandle<uv_async_t> m_async;
    std::pmr::list<async::Executor::Invocation> m_invocations;
    threading::CriticalSection m_invocationsMutex;
    bool m_inWork = false;
};

}  // namespace my