// #my_engine_source_file
#pragma once

#include "my/diag/assert.h"
#include "my/rtti/rtti_utils.h"
#include "my/utils/uni_ptr.h"

#include <concepts>
#include <memory>

namespace my::rtti_detail {

template <typename T>
inline IRefCounted& asRefCounted(T& instance)
{
    static_assert(!std::is_const_v<T>);

    if constexpr (std::is_convertible_v<T*, IRefCounted*>)
    {
        return static_cast<IRefCounted&>(instance);
    }
    else if constexpr (std::is_convertible_v<T*, IRttiObject*>)
    {
        IRefCounted* const rc = static_cast<IRttiObject&>(instance).as<IRefCounted*>();
        MY_DEBUG_FATAL(rc, "Runtime can not find IRefCounted for ({})", rtti::getTypeInfo<T>().getTypeName());
        return *rc;
    }
    else
    {
        IRefCounted* const rc = rtti::staticCast<IRefCounted*>(&instance);
        MY_DEBUG_FATAL(rc, "Runtime can not find IRefCounted for ({})", rtti::getTypeInfo<T>().getTypeName());
        return *rc;
    }
}

template <template <typename, typename...> class UniquePtrT>
struct UniquePtrCastHelper
{
    template <typename U, typename T>
    static UniquePtrT<U> cast(UniquePtrT<T>&& ptr)
    {
        if constexpr (std::is_assignable_v<U&, T&>)
        {
            return UniquePtrT<U>{ptr.release()};
        }
        else
        {
            MY_DEBUG_ASSERT(ptr);
            if (!ptr)
            {
                return {};
            }

            static_assert(rtti::HasTypeInfo<T>);
            static_assert(rtti::HasTypeInfo<U>);

            IRttiObject* const rttiObj = rtti::staticCast<IRttiObject*>(ptr.get());
            MY_DEBUG_FATAL(rttiObj);

            if (auto* const targetPtr = rttiObj->as<U*>(); targetPtr)
            {
                [[maybe_unused]] auto oldPtr = ptr.release();

                return UniquePtrT<U>{targetPtr};
            }

            MY_DEBUG_FAILURE("Can not cast to target type");

            return {};
        }
    }
};

}  // namespace my::rtti_detail

namespace my::rtti {
template <typename T>
struct TakeOwnership
{
    T* const ptr;

    TakeOwnership(T* inPtr) :
        ptr(inPtr)
    {
    }

    TakeOwnership(const TakeOwnership&) = delete;
};

template <typename T>
TakeOwnership(T*) -> TakeOwnership<T>;

}  // namespace my::rtti

namespace my {

template <typename T = IRefCounted>
class Ptr
{
public:
    using type = T;
    using element_type = T;

    Ptr() = default;

    Ptr(std::nullptr_t)
    {
    }

    Ptr(const Ptr<T>& other) :
        m_instance(other.m_instance)
    {
        if (m_instance != nullptr)
        {
            rtti_detail::asRefCounted(*m_instance).addRef();
        }
    }

    Ptr(Ptr<T>&& other) noexcept :
        m_instance(other.giveUp())
    {
    }

    Ptr(T* ptr) :
        m_instance(ptr)
    {
        if (m_instance)
        {
            rtti_detail::asRefCounted(*m_instance).addRef();
        }
    }

    Ptr(const rtti::TakeOwnership<T>& ownership) :
        m_instance(ownership.ptr)
    {
    }

    template <typename U>
    requires(!std::is_same_v<U, T>)
    Ptr(const Ptr<U>& other)

    {
        // static_assert(std::is_convertible_v<U&, T&> || std::is_same_v<T, Com::IRefCountedObject> || std::is_same_v<U, Com::IRefCountedObject>, "Unsafe type cast");
        acquire(other.get());
    }

    template <typename U>
    requires(!std::is_same_v<U, T>)
    Ptr(Ptr<U>&& other)
    {
        // static_assert(std::is_convertible_v<U&, T&> || std::is_same_v<T, Com::IRefCountedObject> || std::is_same_v<U, Com::IRefCountedObject>, "Unsafe type cast");
        moveAcquire(other.giveUp());
    }

    ~Ptr()
    {
        if (m_instance)
        {
            rtti_detail::asRefCounted(*m_instance).releaseRef();
        }
    }

    T* giveUp()
    {
        T* const instance = m_instance;
        m_instance = nullptr;

        return instance;
    }

    T* get() const
    {
        return m_instance;
    }

    void reset(T* ptr = nullptr)
    {
        acquire(ptr);
    }

    Ptr<T>& operator=(const Ptr<T>& other)
    {
        acquire(other.m_instance);
        return *this;
    }

    Ptr<T>& operator=(Ptr<T>&& other) noexcept
    {
        moveAcquire(other.giveUp());
        return *this;
    }

    template <typename U>
    Ptr<T>& operator=(const Ptr<U>& other)
    requires(!std::is_same_v<U, T>)
    {
        U* const instance = other.get();
        acquire<U>(instance);
        return *this;
    }

    template <typename U>
    Ptr<T>& operator=(Ptr<U>&& other)
    requires(!std::is_same_v<U, T>)
    {
        moveAcquire<U>(other.giveUp());
        return *this;
    }

    T& operator*() const
    {
        MY_DEBUG_FATAL(m_instance, "NauPtr<{}> is not dereferenceable", rtti::getTypeInfo<T>().getTypeName());
        return *m_instance;
    }

    T* operator->() const
    {
        MY_DEBUG_FATAL(m_instance, "NauPtr<{}> is not dereferenceable", rtti::getTypeInfo<T>().getTypeName());
        return m_instance;
    }

    explicit operator bool() const
    {
        return m_instance != nullptr;
    }

    bool operator==(std::nullptr_t) const noexcept
    {
        return m_instance == nullptr;
    }

    bool operator==(const Ptr& other) const noexcept
    {
        return m_instance == other.m_instance;
    }

    template <typename U>
    requires(!std::is_same_v<T, U>)
    bool operator==(const Ptr<U>& other) const noexcept
    {
        if (reinterpret_cast<const void*>(m_instance) == reinterpret_cast<void*>(other.get()))
        {
            return true;
        }

        return m_instance != nullptr && other.get() != nullptr &&
               &rtti_detail::asRefCounted(m_instance) == &rtti_detail::asRefCounted(*other.get());
    }

private:
    void acquire(T* newInstance)
    {
        if (T* const prevInstance = std::exchange(m_instance, newInstance); prevInstance)
        {
            rtti_detail::asRefCounted(*prevInstance).releaseRef();
        }

        if (m_instance)
        {
            rtti_detail::asRefCounted(*m_instance).addRef();
        }
    }

    void moveAcquire(T* newInstance)
    {
        if (T* const instance = std::exchange(m_instance, newInstance); instance)
        {
            rtti_detail::asRefCounted(*instance).releaseRef();
        }
    }

    template <typename U>
    requires(!std::is_same_v<U, T>)
    void acquire(U* newInstance)
    {
        if (T* const instance = std::exchange(m_instance, nullptr))
        {
            rtti_detail::asRefCounted(*instance).releaseRef();
        }

        if (newInstance == nullptr)
        {
            // TODO: check is convertible
            return;
        }

        auto& refCounted = rtti_detail::asRefCounted(*newInstance);
        m_instance = refCounted.template as<T*>();
        MY_DEBUG_ASSERT(m_instance, "Expected API not exposed: ({}).", rtti::getTypeInfo<U>().getTypeName());

        if (m_instance != nullptr)
        {
            refCounted.addRef();
        }
    }

    template <typename U>
    requires(!std::is_same_v<U, T>)
    void moveAcquire(U* newInstance)
    {
        if (T* const instance = std::exchange(m_instance, nullptr))
        {
            rtti_detail::asRefCounted(*instance).releaseRef();
        }

        if (newInstance == nullptr)
        {
            return;
        }

        m_instance = rtti_detail::asRefCounted(*newInstance).template as<T*>();
        if (!m_instance)
        {
            rtti_detail::asRefCounted(*newInstance).releaseRef();
            MY_DEBUG_ASSERT(m_instance, "Expected API not exposed: ({}).", rtti::getTypeInfo<U>().getTypeName());
        }
    }

    template <typename U>
    friend std::tuple<U*, void (*)(U*) noexcept, uintptr_t> release_ptr(Ptr<T> smartPtr)
    {
        constexpr uintptr_t PtrMarker = 102;

        if (!smartPtr)
        {
            return {};
        }

        auto release = [](U* value) noexcept
        {
            MY_DEBUG_FATAL(value);
            rtti_detail::asRefCounted(*value).releaseRef();
        };

        T* const ptr = smartPtr.giveUp();
        U* const targetPtr = rtti_detail::asRefCounted(*ptr).template as<U*>();
        MY_DEBUG_ASSERT(targetPtr, "Target interface is not provided");

        return std::tuple{targetPtr, release, PtrMarker};
    }

    template <typename U>
    friend void takes_ownership(Ptr<T>& smartPtr, U* ptr, [[maybe_unused]] uintptr_t marker)
    {
        [[maybe_unused]] constexpr uintptr_t PtrMarker = 102;
        MY_DEBUG_FATAL(marker == PtrMarker);

        if (!ptr)
        {
            smartPtr.reset();
        }
        else
        {
            T* const targetPtr = rtti_detail::asRefCounted(*ptr).template as<T*>();
            smartPtr = Ptr<T>{rtti::TakeOwnership{targetPtr}};
        }
    }

    T* m_instance = nullptr;
};

template <typename T>
Ptr(T*) -> Ptr<T>;

template <typename T>
Ptr(const rtti::TakeOwnership<T>&) -> Ptr<T>;

template <typename T>
UniPtr(Ptr<T>) -> UniPtr<T>;

}  // namespace my
namespace my::rtti {
template <typename U, typename T>
my::Ptr<U> pointer_cast(my::Ptr<T>&& ptr)
{
    return my::Ptr<U>{std::move(ptr)};
}

template <typename U, typename T>
std::unique_ptr<U> pointer_cast(std::unique_ptr<T>&& ptr)
{
    return rtti_detail::UniquePtrCastHelper<std::unique_ptr>::cast<U>(std::move(ptr));
}

}  // namespace my::rtti
