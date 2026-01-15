// #my_engine_source_file
#include "runtime_executor.h"

#include "my/threading/lock_guard.h"
#include "runtime/kernel_runtime_impl.h"

namespace my {

RuntimeThreadExecutor::RuntimeThreadExecutor() :
    m_invocations(getKernelRuntimeImpl().getUvHandleAllocator().getMemoryResource())
{
    uv_async_init(getKernelRuntimeImpl().uv(), m_async, [](uv_async_t* const handle) noexcept
    {
        RuntimeThreadExecutor& self = *static_cast<RuntimeThreadExecutor*>(handle->data);
        const Executor::InvokeGuard invokeGuard{self};

        while (true)
        {
            decltype(m_invocations) invocations{getKernelRuntimeImpl().getUvHandleAllocator().getMemoryResource()};

            {
                const std::lock_guard lock(self.m_invocationsMutex);
                if (self.m_invocations.empty())
                {
                    return;
                }

                self.m_inWork = true;
                invocations.splice(invocations.begin(), self.m_invocations);
            }

            scope_on_leave
            {
                const std::lock_guard lock(self.m_invocationsMutex);
                self.m_inWork = false;

            };
            for (Executor::Invocation& invocation : invocations)
            {
                Executor::invoke(self, std::move(invocation));
            }
        }
    });

    uv_handle_set_data(m_async, this);
    getKernelRuntimeImpl().setHandleAsInternal(m_async);
}

void RuntimeThreadExecutor::waitAnyActivity() noexcept
{
}

void RuntimeThreadExecutor::scheduleInvocation(Invocation invocation) noexcept
{
    {
        const std::lock_guard lock(m_invocationsMutex);
        m_invocations.emplace_back(std::move(invocation));
    }

    uv_async_send(m_async);
}

bool RuntimeThreadExecutor::hasWorks()
{
    const std::lock_guard lock(m_invocationsMutex);
    return !m_invocations.empty() || m_inWork;

}

}  // namespace my