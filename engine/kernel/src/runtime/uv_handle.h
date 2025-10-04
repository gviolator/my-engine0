// #my_engine_source_file
#pragma once
#include "my/diag/assert.h"
#include "my/utils/type_list/append.h"
#include "my/utils/type_list/type_list.h"

#include <uv.h>

#include <type_traits>

namespace my {

using KnownUvHandles = TypeList<
    uv_handle_t,
    uv_async_t,
    uv_stream_t,
    uv_timer_t,
    uv_tcp_t,
    uv_pipe_t,
    uv_tty_t,
    uv_udp_t>;

namespace kernel_detail {

template <typename Handle, typename T = KnownUvHandles>
struct IsKnownUvHandleHelper;

template <typename Handle, typename... T>
struct IsKnownUvHandleHelper<Handle, TypeList<T...>>
{
    static constexpr bool value = (std::is_same_v<Handle, T> || ...);
};

/**
 */
class UvHandleBase
{
public:
    UvHandleBase() = default;
    UvHandleBase(uv_handle_type);
    UvHandleBase(const UvHandleBase&) = delete;
    ~UvHandleBase();

    explicit operator bool() const;
    void setData(void*);
    void* data() const;
    void reset();

protected:
    static bool isAssignable(const std::type_info& thisHandleType, const std::type_info& srcHandleType);

    void resetInternal(uv_handle_t* handle = nullptr);

    uv_handle_t* m_handle = nullptr;

    template <typename X>
    friend class UvHandle;
};

}  // namespace kernel_detail

template <typename T>
inline constexpr bool IsKnownUvHandle = kernel_detail::IsKnownUvHandleHelper<T>::value;

template <typename T>
requires(IsKnownUvHandle<T>)
consteval uv_handle_type getUvHandleType()
{
    if constexpr (std::is_same_v<T, uv_async_t>)
    {
        return UV_ASYNC;
    }
    else if constexpr (std::is_same_v<T, uv_timer_t>)
    {
        return UV_TIMER;
    }
    else if constexpr (std::is_same_v<T, uv_tcp_t>)
    {
        return UV_TCP;
    }
    else if constexpr (std::is_same_v<T, uv_udp_t>)
    {
        return UV_UDP;
    }
    else if constexpr (std::is_same_v<T, uv_tty_t>)
    {
        return UV_TTY;
    }
    else if constexpr (std::is_same_v<T, uv_pipe_t>)
    {
        return UV_NAMED_PIPE;
    }

    return UV_UNKNOWN_HANDLE;
}

template <typename T>
inline constexpr bool IsUvStreamCompatible =
    std::is_same_v<T, uv_tcp_t> ||
    std::is_same_v<T, uv_pipe_t> ||
    std::is_same_v<T, uv_tty_t>;

template <typename Dst, typename Src>
inline constexpr bool IsAssignableUvHandles =
    std::is_same_v<Dst, Src> ||
    std::is_same_v<Dst, uv_handle_t> ||
    (std::is_same_v<Dst, uv_stream_t> && IsUvStreamCompatible<Src>);

/**
 */
template <typename Target, typename Handle>
requires(IsKnownUvHandle<Target> && IsKnownUvHandle<Handle> && IsAssignableUvHandles<Handle, Target>)
inline Target* castUvHandle(Handle* source)
{
    return reinterpret_cast<Target>(source);
}

/**
 */
template <typename Handle = uv_handle_t>
class UvHandle : public kernel_detail::UvHandleBase
{
    static_assert(IsKnownUvHandle<Handle>);
    using Base = kernel_detail::UvHandleBase;

public:
    UvHandle(std::nullptr_t) :
        Base()
    {
    }

    UvHandle() :
        Base(getUvHandleType<Handle>())
    {
    }

    UvHandle(UvHandle&& other)
    {
        m_handle = std::exchange(other.m_handle, nullptr);
    }

    template <typename U>
    requires(IsAssignableUvHandles<Handle, U>)
    UvHandle(UvHandle<U>&& other)
    {
        MY_DEBUG_ASSERT(Base::isAssignable(typeid(Handle), typeid(U)));
        m_handle = std::exchange(other.m_handle, nullptr);
    }

    UvHandle& operator=(UvHandle&& other)
    {
        this->resetInternal(std::exchange(other.m_handle, nullptr));
        return *this;
    }

    template <typename U>
    requires(IsAssignableUvHandles<Handle, U>)
    UvHandle& operator=(UvHandle<U>&& other)
    {
        MY_DEBUG_ASSERT(Base::isAssignable(typeid(Handle), typeid(U)));
        this->resetInternal(std::exchange(other.m_handle, nullptr));
        return *this;
    }

    Handle* operator->() const
    {
        MY_DEBUG_ASSERT(m_handle);
        return reinterpret_cast<Handle*>(m_handle);
    }

    operator Handle*() const
    requires(!std::is_same_v<Handle, uv_handle_t>)

    {
        MY_DEBUG_ASSERT(m_handle);
        // DEBUG_CHECK(m_handle->type == UV_ASYNC || Runtime::isRuntimeThread())

        return reinterpret_cast<Handle*>(m_handle);
    }

    operator uv_handle_t*() const
    {
        MY_DEBUG_ASSERT(m_handle);
        // DEBUG_CHECK(m_handle->type == UV_ASYNC || Runtime::isRuntimeThread())

        return m_handle;
    }

    template <typename U>
    requires(IsAssignableUvHandles<Handle, U>)
    U* as() const
    {
        MY_DEBUG_ASSERT(m_handle);
        MY_DEBUG_ASSERT(UvHandleBase::isAssignable(typeid(U), typeid(Handle)));

        return reinterpret_cast<U*>(m_handle);
    }


    template <typename>
    friend class UvHandle;
};

inline size_t getUvHandleMaxSize()
{
    const auto helper = []<typename... T>(TypeList<T...>)
    {
        return std::max({sizeof(T)...});
    };

    using UvTypes = type_list::Append<KnownUvHandles, uv_shutdown_t>;
    return helper(UvTypes{});
}

}  // namespace my
