// #my_engine_source_file
#include "my/async/task.h"
#include "my/network/network.h"
#include "my/runtime/internal/kernel_runtime.h"
#include "my/test/helpers/runtime_guard.h"

#if 0
namespace my::test {

class TestNetworkClient : public testing::Test
{
protected:
    virtual void TearDown()
    {
        // auto processShutdown = m_runtime->shutdown();
        // while (processShutdown())
        // {
        //     std::this_thread::yield();
        // }
    }

    // KernelRuntimePtr m_runtime = createKernelRuntime();
    //RuntimeGuard::Ptr m_runtime = RuntimeGuard::create();
};

TEST_F(TestNetworkClient, Test1)
{
    GTEST_SKIP_("Runtime failure");
    auto t = [](auto& self) -> async::Task<>
    {
        co_await self.m_runtime->getKRuntime().getRuntimeExecutor();
        std::cout << "In runtime\n";
    }(*this);

    async::wait(t);

    std::cout << "Complete\n";
}

}  // namespace my::test

#endif