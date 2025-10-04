// #my_engine_source_file
#pragma once
#include "my/network/address.h"
#include "my/rtti/rtti_impl.h"

namespace my::network {

/**
*/
class InetAddress final : public IAddress
{
    MY_REFCOUNTED_CLASS(InetAddress, IAddress)

public:
    InetAddress(sockaddr sockAddr);

    AddressKind getKind() const override;

    size_t getSize() const override;

    IAddress* getAt(size_t i) const override;

    const sockaddr* getSockAddr() const;

    unsigned short getFamily() const;

private:
    sockaddr m_sockAddr;
};

}  // namespace my::network
