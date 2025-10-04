// #my_engine_source_file
#include "my/network/network.h"
#include "my/test/helpers/runtime_guard.h"
#include "network_test_utils.h"

using namespace my::async;
using namespace my::network;
using namespace std::chrono_literals;

namespace my::test {

class TestListener : public testing::TestWithParam<network::Address>
{
protected:
    Address address() const
    {
        return GetParam();
    }

    RuntimeGuard::Ptr m_runtime = RuntimeGuard::create();
};

TEST_P(TestListener, Create)
{
    Result<Ptr<IListener>> listener = async::waitResult(network::listen(address()));
    ASSERT_TRUE(listener);
}

TEST_P(TestListener, FailListenOnBusyAddress)
{
    auto task = [](Address address) -> Task<testing::AssertionResult>
    {
        Ptr<IListener> listener = co_await network::listen(address);
        if (!listener)
        {
            co_return testing::AssertionFailure() << "Fail to first listener";
        }

        Result<Ptr<IListener>> secondListenResult = co_await network::listen(address).doTry();
        if (secondListenResult)
        {
            co_return testing::AssertionFailure() << "Expected second listen failure";
        }

        co_return testing::AssertionSuccess;
    }(address());

    async::wait(task);
    ASSERT_TRUE(*task);
}

TEST_P(TestListener, ShutdownWhileAccepting)
{
    auto task = [](Address address) -> Task<bool>
    {
        Ptr<IListener> listener = co_await network::listen(address);
        io::AsyncStreamPtr client = co_await listener->accept();

        co_return client == nullptr;
    }(address());

    []() -> Task<>
    {
        co_await 2ms;
        getKernelRuntime().shutdown();

    }().detach();

    async::wait(task);

    ASSERT_FALSE(task.isRejected());
    ASSERT_TRUE(*task);
}

INSTANTIATE_TEST_SUITE_P(Default, TestListener, testing::ValuesIn(testAddressList()));

}  // namespace my::test
