// #my_engine_source_file
#include "disposable_runtime_object.h"
#include "my/async/task.h"
#include "my/runtime/internal/kernel_runtime.h"

namespace my {

bool DisposableRuntimeObject::isDisposed() const
{
    return m_isDisposed.load(std::memory_order_relaxed);
}

void DisposableRuntimeObject::doDispose(bool waitFor, IRefCounted& refCountedSelf, void (*callback)(IRefCounted&) noexcept)
{
    if (m_isDisposed.exchange(true) == true)
    {
        return;
    }

    if (getKernelRuntime().isRuntimeThread())
    {
        callback(refCountedSelf);
        return;
    }

    if (waitFor)
    {
        async::Task<> task = [](IRefCounted& self, decltype(callback) callback) -> async::Task<>
        {
            co_await getKernelRuntime().getRuntimeExecutor();
            callback(self);
        }(refCountedSelf, callback);
        async::wait(task);
    }
    else
    {
        // if called not from destructor (i.e. from dispose)
        // there is no need to wait task completion, instead
        // can use addRef/releaseRef to guarantee that instance alive during call.
        // In that case instance can be destroyed within scope_on_leave inside callback.
        async::Task<> task = [](IRefCounted& self, auto callback) -> async::Task<>
        {
            self.addRef();
            scope_on_leave
            {
                self.releaseRef();
            };

            co_await getKernelRuntime().getRuntimeExecutor();
            callback(self);
        }(refCountedSelf, callback);
        task.detach();
    }
}

}  // namespace my