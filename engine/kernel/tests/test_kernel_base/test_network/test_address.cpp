// #my_engine_source_file
#include "my/network/address.h"

using namespace my::network;

namespace my::test {

TEST(TestNetworkAddress, Inet4FromString)
{
    Result<Address> addr = AddressFromString("inet://127.0.0.1:7077");
    ASSERT_TRUE(addr);
    ASSERT_EQ(addr->getKind(), AddressKind::Inet);
}

TEST(TestNetworkAddress, Inet4NoPortFromString)
{
    Result<Address> addr = AddressFromString("inet://127.0.0.1");
    ASSERT_TRUE(addr);
    ASSERT_EQ(addr->getKind(), AddressKind::Inet);
}

TEST(TestNetworkAddress, Inet4InvalidString)
{
    Result<Address> addr = AddressFromString("inet://127.0");
    ASSERT_FALSE(addr);
}

TEST(TestNetworkAddress, Inet6FromString)
{
    Result<Address> addr = AddressFromString("inet6://[::1]:8088");
    ASSERT_TRUE(addr);
    ASSERT_EQ(addr->getKind(), AddressKind::Inet6);
}

TEST(TestNetworkAddress, Inet6NoPortFromString)
{
    {
        Result<Address> addr = AddressFromString("inet6://[::1]");
        ASSERT_TRUE(addr);
        ASSERT_EQ(addr->getKind(), AddressKind::Inet6);
    }

    {
        Result<Address> addr = AddressFromString("inet6://::1");
        ASSERT_TRUE(addr);
        ASSERT_EQ(addr->getKind(), AddressKind::Inet6);
    }
}

}  // namespace my::test
