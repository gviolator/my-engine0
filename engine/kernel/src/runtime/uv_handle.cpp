// #my_engine_source_file
#include "my/async/task.h"
#include "runtime/kernel_runtime_impl.h"
#include "uv_handle.h"

namespace my {

static_assert(IsAssignableUvHandles<uv_handle_t, uv_tcp_t>);
static_assert(IsAssignableUvHandles<uv_handle_t, uv_udp_t>);
static_assert(IsAssignableUvHandles<uv_handle_t, uv_pipe_t>);
static_assert(IsAssignableUvHandles<uv_handle_t, uv_tty_t>);
static_assert(IsAssignableUvHandles<uv_handle_t, uv_timer_t>);
static_assert(IsAssignableUvHandles<uv_stream_t, uv_tcp_t>);
static_assert(IsAssignableUvHandles<uv_stream_t, uv_pipe_t>);
static_assert(IsAssignableUvHandles<uv_stream_t, uv_tty_t>);
static_assert(!IsAssignableUvHandles<uv_stream_t, uv_udp_t>);
static_assert(!IsAssignableUvHandles<uv_stream_t, uv_timer_t>);

static_assert(!IsUvStreamCompatible<uv_timer_t>);
static_assert(!IsUvStreamCompatible<uv_udp_t>);
static_assert(IsUvStreamCompatible<uv_tcp_t>);
static_assert(IsUvStreamCompatible<uv_pipe_t>);
static_assert(IsUvStreamCompatible<uv_tty_t>);

}  // namespace my

namespace my::kernel_detail {

namespace {

constexpr size_t UvHandleBadSize = static_cast<size_t>(-1);

inline void* allocateHandleMem(size_t size)
{
    void* const mem = getKernelRuntimeImpl().getUvHandleAllocator().alloc(size);
    MY_DEBUG_FATAL(mem);
#ifndef NDEBUG
    memset(mem, 0, size);
#endif
    return mem;
}

inline void freeHandleMem(void* mem)
{
    getKernelRuntimeImpl().getUvHandleAllocator().free(mem);
}

template <typename T>
T* allocateHandle()
{
    void* const mem = allocateHandleMem(sizeof(T));
    return static_cast<T*>(mem);
}

inline uv_handle_t* allocateHandle(uv_handle_type type)
{
    const size_t size = uv_handle_size(type);
    MY_DEBUG_FATAL(size != UvHandleBadSize);

    void* const mem = allocateHandleMem(size);
    return static_cast<uv_handle_t*>(mem);
}

void freeHandle(uv_handle_t* handle)
{
    MY_DEBUG_ASSERT(handle);
    freeHandleMem(handle);
}

void closeAndFreeUvHandle(uv_handle_t* handle)
{
    if (!handle)
    {
        return;
    }
    MY_DEBUG_ASSERT(uv_is_closing(handle) == 0);

    uv_handle_set_data(handle, nullptr);

    const auto isStreamCompatible = [](uv_handle_type t)
    {
        return t == UV_TTY || t == UV_TCP || t == UV_NAMED_PIPE;
    };

    if (const uv_handle_type type = uv_handle_get_type(handle); isStreamCompatible(type))
    {
        uv_stream_t* const stream = reinterpret_cast<uv_stream_t*>(handle);
        if (uv_is_writable(stream) != 0 && uv_is_active(handle) != 0)
        {
            uv_shutdown_t* const request = allocateHandle<uv_shutdown_t>();

            const auto res = uv_shutdown(request, stream, [](uv_shutdown_t* request, [[maybe_unused]] int status) noexcept
            {
                uv_handle_t* const handle = reinterpret_cast<uv_handle_t*>(request->handle);
                freeHandleMem(request);
                uv_close(handle, freeHandle);
            });

            if (res == 0)
            {
                // shutdown is pending:
                // request and handle will be closed inside shutdown callback;
                return;
            }

            freeHandleMem(request);
        }
    }

    uv_close(handle, freeHandle);
}

}  // namespace

UvHandleBase::UvHandleBase(uv_handle_type type)
{
    if (type != UV_UNKNOWN_HANDLE)
    {
        m_handle = allocateHandle(type);
    }
}

UvHandleBase::~UvHandleBase()
{
    resetInternal();
}

void UvHandleBase::reset()
{
    resetInternal(nullptr);
}

void UvHandleBase::resetInternal(uv_handle_t* newHandle)
{
#ifndef NDEBUG
    if (m_handle && newHandle)
    {
        const uv_handle_type currHandleType = uv_handle_get_type(m_handle);
        const uv_handle_type newHandleType = uv_handle_get_type(newHandle);
        MY_DEBUG_ASSERT(currHandleType == newHandleType);
    }
#endif

    uv_handle_t* const prevHandle = std::exchange(m_handle, newHandle);

    if (getKernelRuntimeImpl().isRuntimeThread())
    {
        closeAndFreeUvHandle(prevHandle);
    }
    else
    {
        [](uv_handle_t* handle) -> async::Task<>
        {
            co_await getKernelRuntimeImpl().getRuntimeExecutor();
            closeAndFreeUvHandle(handle);
        }(prevHandle).detach();
    }
}

UvHandleBase::operator bool() const
{
    return m_handle != nullptr;
}

void UvHandleBase::setData(void* data)
{
    MY_DEBUG_ASSERT(m_handle);
    uv_handle_set_data(m_handle, data);
}

void* UvHandleBase::data() const
{
    MY_DEBUG_ASSERT(m_handle);
    return uv_handle_get_data(m_handle);
}

bool UvHandleBase::isAssignable(const std::type_info& thisHandleType, const std::type_info& srcHandleType)
{
    if (thisHandleType == typeid(uv_handle_t))
    {
        return true;
    }

    if (thisHandleType == typeid(uv_stream_t))
    {
        return srcHandleType == typeid(uv_tcp_t) || srcHandleType == typeid(uv_pipe_t) || srcHandleType == typeid(uv_tty_t);
    }

    return thisHandleType == srcHandleType;
}

}  // namespace my::kernel_detail
