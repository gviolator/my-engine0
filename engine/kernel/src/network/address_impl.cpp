// #my_engine_source_file
#include "address_impl.h"
#include "my/async/task.h"
#include "my/rtti/rtti_impl.h"
#include "my/utils/string_utils.h"

using namespace std::literals;

namespace my::network {

namespace {

constexpr auto DefaultMatchOptions = std::regex_constants::ECMAScript | std::regex_constants::icase;
constexpr auto Kind_Inet = "inet"sv;
constexpr auto Kind_Inet6 = "inet6"sv;
constexpr auto Kind_IPC = "ipc"sv;

Result<Ptr<IAddress>> CreateInetAddress(std::string_view address)
{
    constexpr auto Ipv4Re = R"-(^(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})(:(\d{1,5}))?$)-";
    constexpr auto Ipv4AnyRe = R"-(^\*(:(\d{1,5}))?$)-";

    sockaddr sockAddr;
    memset(&sockAddr, 0, sizeof(sockAddr));
    struct sockaddr_in* const sockAddrIn = reinterpret_cast<sockaddr_in*>(&sockAddr);
    sockAddrIn->sin_family = AF_INET;

    std::match_results<std::string_view::iterator> match;

    if (std::regex_match(address.begin(), address.end(), match, std::regex{Ipv4AnyRe, DefaultMatchOptions}))
    {
        MY_DEBUG_FATAL(match.size() >= 2);
        if (const auto& portMatch = match[2]; portMatch.matched)
        {
            const std::string portStr{portMatch.first, portMatch.second};
            sockAddrIn->sin_port = htons(static_cast<unsigned short>(std::stoi(portStr)));
        }

        sockAddrIn->sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else if (std::regex_match(address.begin(), address.end(), match, std::regex{Ipv4Re, DefaultMatchOptions}))
    {
        MY_DEBUG_FATAL(match.size() >= 4);
        if (const auto& portMatch = match[3]; portMatch.matched)
        {
            const std::string portStr{portMatch.first, portMatch.second};
            sockAddrIn->sin_port = htons(static_cast<unsigned short>(std::stoi(portStr)));
        }

        const std::string_view ipAddr{match[1].first, match[1].second};
        if (inet_pton(AF_INET, std::string{ipAddr}.c_str(), &sockAddrIn->sin_addr) != 1)
        {
            return MakeError("Fail to parse inet addr:({})", ipAddr);
        }
    }
    else
    {
        return MakeError("Invalid ipv4 address: ({})", address);
    }

    return rtti::createInstance<InetAddress>(sockAddr);
}

Result<Ptr<IAddress>> CreateInet6Address(std::string_view address)
{
    constexpr auto Ipv6Re = R"-(^([0-9a-fA-F:]*))-";
    constexpr auto Ipv6WithPortRe = R"-(^\[([0-9a-fA-F:]*)\](:(\d{1,5}))?$)-";

    std::string_view ipAddr;
    unsigned short port = 0;

    std::match_results<std::string_view::iterator> match;
    if (std::regex_match(address.begin(), address.end(), match, std::regex{Ipv6WithPortRe, DefaultMatchOptions}))
    {
        MY_DEBUG_FATAL(match.size() >= 4);
        ipAddr = std::string_view{match[1].first, match[1].second};

        if (const auto& portMatch = match[3]; portMatch.matched)
        {
            const std::string portStr{portMatch.first, portMatch.second};
            MY_DEBUG_ASSERT(!portStr.empty());
            port = static_cast<unsigned short>(std::stoi(portStr));
        }
    }
    else if (std::regex_match(address.begin(), address.end(), match, std::regex{Ipv6Re, DefaultMatchOptions}))
    {
        MY_DEBUG_FATAL(match.size() >= 2);
        ipAddr = std::string_view{match[1].first, match[1].second};
    }
    else
    {
        return MakeError("Invalid ipv6 address: ({})", address);
    }

    sockaddr sockAddr;
    memset(&sockAddr, 0, sizeof(sockAddr));
    {
        sockaddr_in6* const sockAddrIn = reinterpret_cast<sockaddr_in6*>(&sockAddr);

        sockAddrIn->sin6_family = AF_INET6;
        sockAddrIn->sin6_port = htons(port);

        if (inet_pton(AF_INET6, std::string{ipAddr}.c_str(), &sockAddrIn->sin6_addr) != 1)
        {
            return MakeError("Fail to parse inet6 addr:({})", ipAddr);
        }
    }

    return rtti::createInstance<InetAddress>(sockAddr);
}

Result<Address> ParseIpcAddress(std::string_view line)
{
    return Address{};
}

}  // namespace

InetAddress::InetAddress(sockaddr sockAddr) :
    m_sockAddr(sockAddr)
{
    MY_DEBUG_ASSERT(m_sockAddr.sa_family == AF_INET || m_sockAddr.sa_family == AF_INET6);
}

AddressKind InetAddress::getKind() const
{
    const auto family = m_sockAddr.sa_family;
    return family == AF_INET ? AddressKind::Inet : AddressKind::Inet6;
}

size_t InetAddress::getSize() const
{
    return 1;
}

IAddress* InetAddress::getAt(size_t i) const
{
    MY_DEBUG_ASSERT(i == 0);

    if (i != 0)
    {
        return nullptr;
    }

    auto mutableThis = const_cast<InetAddress*>(this);
    return mutableThis;
}

const sockaddr* InetAddress::getSockAddr() const
{
    return &m_sockAddr;
}

unsigned short InetAddress::getFamily() const
{
    return m_sockAddr.sa_family;
}

Address::Address() = default;

Address::Address(const Address&) = default;

Address::Address(Address&&) = default;

Address::Address(Ptr<IAddress>&& addrPtr) :
    m_address(std::move(addrPtr))
{
}

Address::Address(IAddress* address)
{
    MY_DEBUG_ASSERT(address, "This constructor not supposed to accept nullptr for address");
    m_address = address;
}

Address& Address::operator=(const Address&) = default;

Address& Address::operator=(Address&&) = default;

Address::operator bool() const
{
    return static_cast<bool>(m_address);
}

AddressKind Address::getKind() const
{
    return m_address ? m_address->getKind() : AddressKind::Unknown;
}

size_t Address::getSize() const
{
    return m_address ? m_address->getSize() : 0;
}

IAddress* Address::operator[](size_t i) const
{
    MY_DEBUG_ASSERT(m_address);
    MY_DEBUG_ASSERT(i < getSize());
    if (!m_address || i >= getSize())
    {
        return nullptr;
    }

    return m_address->getAt(i);
}

Address::iterator Address::begin() const
{
    if (!m_address || m_address->getSize() == 0)
    {
        return {};
    }

    return iterator{this, 0};
}

Address::iterator Address::end() const
{
    return {};
}

AddressIterator::AddressIterator(const Address* addr_in, size_t index_in) :
    address(addr_in),
    index(index_in)
{
}

AddressIterator& AddressIterator::operator++()
{
    MY_DEBUG_ASSERT(address, "AddressIterator is not dereferenceable");

    if (index == address->getSize())
    {  // last iterator turns into end
        address = nullptr;
        index = std::numeric_limits<size_t>::max();
    }
    else
    {
        ++index;
    }

    return *this;
}

AddressIterator AddressIterator::operator++(int)
{
    AddressIterator copy{*this};
    return ++copy;
}

IAddress* AddressIterator::operator*() const
{
    MY_DEBUG_ASSERT(address, "AddressIterator is not dereferenceable");
    return address ? (*address)[index] : nullptr;
}

bool operator==(const AddressIterator& lhs, const AddressIterator& rhs)
{
    return lhs.address == rhs.address && lhs.index == rhs.index;
}

bool operator!=(const AddressIterator& lhs, const AddressIterator& rhs)
{
    return lhs.address != rhs.address || lhs.index != rhs.index;
}

Result<Address> AddressFromString(std::string_view addressStr)
{
    std::match_results<std::string_view::iterator> match;
    if (!std::regex_match(addressStr.begin(), addressStr.end(), match, std::regex{"^([A-Za-z0-9\\+_-]+)://(.+)$", DefaultMatchOptions}))
    {
        return MakeError("Invalid address:({})", addressStr);
    }
    MY_DEBUG_FATAL(match.size() >= 3);

    const std::string_view kind{match[1].first, match[1].second};
    const std::string_view address{match[2].first, match[2].second};
    Result<Ptr<IAddress>> addressResult;

    if (strings::icaseEqual(kind, Kind_Inet))
    {
        addressResult = CreateInetAddress(address);
    }
    else if (strings::icaseEqual(kind, Kind_Inet6))
    {
        addressResult = CreateInet6Address(address);
    }
    else if (strings::icaseEqual(kind, Kind_IPC))
    {
        return ParseIpcAddress(address);
    }
    else
    {
        return MakeError("Unknown network address kind:({})", kind);
    }

    CheckResult(addressResult);
    return Address{std::move(*addressResult)};
}

async::Task<Address> ResolveAddress(std::string_view)
{
    co_return Address{};
}

}  // namespace my::network
