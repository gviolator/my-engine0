// #my_engine_source_file
#include "my/async/task.h"
#include "my/network/network.h"
#include "my/runtime/internal/kernel_runtime.h"
#include "my/test/helpers/runtime_guard.h"
#include "my/threading/event.h"

namespace my::test {

class TestNetworkClient : public testing::Test
{
protected:
    virtual void TearDown()
    {
        m_runtime.reset();
    }

    RuntimeGuard::Ptr m_runtime = RuntimeGuard::create();
};

TEST_F(TestNetworkClient, Test1)
{
    threading::Event serverReady;
    auto serverTask = [](threading::Event& signal) -> async::Task<>
    {
        Ptr<network::IListener> listener = co_await network::listen(*network::addressFromString("inet://*:12345"));
        signal.set();
        io::AsyncStreamPtr client = co_await listener->accept();
        std::cout << "Accepted\n";

        Buffer data{10};
        co_await client->write(data.toReadOnly());

    }(serverReady);

    serverReady.wait();

    async::Task<> clientTask = []() -> async::Task<>
    {
        io::AsyncStreamPtr client = co_await network::connect(*network::addressFromString("inet://127.0.0.1:12345"), {}, Expiration::never());
        if (!client)
        {
            std::cout << "Bad client\n";
            co_return;
        }

        std::cout << "Connected\n";
        Buffer data = co_await client->read();

        std::cout << std::format("Incoming ({}) bytes", data.size()) << std::endl;

    }();


    // auto t = [](auto& self) -> async::Task<>
    //{
    //     co_await self.m_runtime->getKRuntime().getRuntimeExecutor();
    //     std::cout << "In runtime\n";
    // }(*this);

    async::wait(serverTask);
    async::wait(clientTask);

    std::cout << "Complete\n";
}

}  // namespace my::test
