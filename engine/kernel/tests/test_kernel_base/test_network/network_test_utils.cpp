// #my_engine_source_file
#include "network_test_utils.h"

using namespace my::network;

namespace my::test {

std::vector<Address> testAddressList()
{
    int port = 8745;

    return {
        *addressFromString(std::format("inet://127.0.0.1:{}", port))};
}
}  // namespace my::test
