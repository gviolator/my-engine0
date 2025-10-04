// #my_engine_source_file
#pragma once
#include "my/async/task_base.h"
#include "my/kernel/kernel_config.h"
#include "my/rtti/ptr.h"
#include "my/rtti/rtti_object.h"
#include "my/utils/result.h"

#include <iterator>
#include <limits>

namespace my::network {

enum class AddressKind
{
    Unknown,
    Collection,
    Inet,
    Inet6,
    Ipc
};

/**
 */
struct IAddress : IRefCounted
{
    MY_INTERFACE(my::network::IAddress, IRefCounted)

    virtual ~IAddress() = default;
    virtual AddressKind getKind() const = 0;
    virtual size_t getSize() const = 0;
    virtual IAddress* getAt(size_t i) const = 0;
};

/**
 */
struct MY_KERNEL_EXPORT AddressIterator
{
    using iterator_category = std::forward_iterator_tag;
    using value_type = IAddress*;
    using difference_type = size_t;

    AddressIterator() = default;
    AddressIterator(const class Address*, size_t index);

    AddressIterator& operator++();
    AddressIterator operator++(int);
    IAddress* operator*() const;

private:
    friend bool operator==(const AddressIterator&, const AddressIterator&);
    friend bool operator!=(const AddressIterator&, const AddressIterator&);

    const class Address* address = nullptr;
    size_t index = std::numeric_limits<size_t>::max();
};

/**
 */
class MY_KERNEL_EXPORT Address
{
public:
    using iterator = AddressIterator;

    Address();
    Address(const Address&);
    Address(Address&&);
    Address(Ptr<IAddress>&&);
    Address(IAddress*);

    Address& operator=(const Address&);
    Address& operator=(Address&&);

    explicit operator bool() const;

    AddressKind getKind() const;
    size_t getSize() const;
    IAddress* operator[](size_t) const;

    iterator begin() const;
    iterator end() const;

private:
    Ptr<IAddress> m_address;
};

MY_KERNEL_EXPORT Result<Address> addressFromString(std::string_view);

MY_KERNEL_EXPORT async::Task<Address> resolveAddress(std::string_view);

}  // namespace my::network
