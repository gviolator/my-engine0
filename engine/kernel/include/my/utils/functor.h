// #my_engine_source_file

#pragma once

#include "my/diag/assert.h"
#include "my/memory/allocator.h"
#include "my/memory/runtime_stack.h"
#include "my/meta/function_info.h"
#include "my/rtti/type_info.h"

#include <type_traits>

namespace my {

template <bool, typename R, typename P>
struct IGenericInvokable;

template <bool NoExcept, typename Result, typename... Parameters>
struct MY_ABSTRACT_TYPE IGenericInvokable<NoExcept, Result, TypeList<Parameters...>>
{
    virtual ~IGenericInvokable() = default;

    virtual Result operator()(Parameters...) noexcept(NoExcept) = 0;
};

namespace kernel_detail {

template <typename Callable, bool NoExcept, typename Result, typename Parameters>
class CallableImpl;

template <typename Callable, bool NoExcept, typename Result, typename... Parameters>
class CallableImpl<Callable, NoExcept, Result, TypeList<Parameters...>> final : public IGenericInvokable<NoExcept, Result, TypeList<Parameters...>>
{
public:
    CallableImpl(Callable&& callable) :
        m_callable(std::move(callable))
    {
        static_assert(std::is_invocable_r_v<Result, Callable, Parameters...>, "Invalid functor argument");
        static_assert(!NoExcept || noexcept(callable(std::declval<Parameters>()...)), "Functor must specify noexcept specification");
    }

    Result operator()(Parameters... p) noexcept(NoExcept) override
    {
        return m_callable(std::forward<Parameters>(p)...);
    }

private:
    Callable m_callable;
};

}  // namespace kernel_detail

template <bool, typename, typename>
struct FunctorImpl;

template <bool NoExcept, typename R, typename... P>
struct FunctorImpl<NoExcept, R, TypeList<P...>>
{
    using InvokableItf = IGenericInvokable<NoExcept, R, TypeList<P...>>;

    InvokableItf* m_invocable = nullptr;
    void* m_mem = nullptr;
    IAllocator* m_allocator = nullptr;

    FunctorImpl() = default;

    FunctorImpl(std::nullptr_t) noexcept :
        FunctorImpl()
    {
    }

    template <typename Func>
    FunctorImpl(Func f, IAllocator* allocator) :
        m_allocator(allocator)
    {
        using CallableImpl = kernel_detail::CallableImpl<Func, NoExcept, R, TypeList<P...>>;

        static_assert(std::is_invocable_r_v<R, Func, P...>, "Functor has unacceptable parameters");
        static_assert(std::is_assignable_v<InvokableItf&, CallableImpl&>);

        if (!m_allocator)
        {
            m_allocator = getDefaultAllocatorPtr();
        }

        MY_DEBUG_FATAL(m_allocator);

        m_mem = m_allocator->alloc(sizeof(CallableImpl));
        m_invocable = new(m_mem) CallableImpl{std::move(f)};
    }

    FunctorImpl(FunctorImpl&& other) noexcept :
        m_invocable(std::exchange(other.m_invocable, nullptr)),
        m_allocator(std::exchange(other.m_allocator, nullptr))
    {
    }

    FunctorImpl(const FunctorImpl&) = delete;
    ~FunctorImpl()
    {
        reset();
    }
#if 0
    FunctorImpl& operator=(FunctorImpl&& other) noexcept
    {
        reset();
        m_invocable = std::exchange(other.m_invocable, nullptr);
        m_allocator = std::exchange(other.m_allocator, nullptr);

        return *this;
    }

    FunctorImpl& operator=(std::nullptr_t)
    {
        reset();
        return *this;
    }
#endif
    R operator()(P... args) const noexcept(NoExcept)
    {
        MY_DEBUG_ASSERT(m_invocable);
        return (*m_invocable)(std::forward<P>(args)...);
    }

    explicit operator bool() const noexcept
    {
        return m_invocable != nullptr;
    }

protected:
    void reset()
    {
        if (m_invocable)
        {
            auto* const ptr = std::exchange(m_invocable, nullptr);
            IAllocator* const allocator = std::exchange(m_allocator, nullptr);
            MY_DEBUG_FATAL(allocator);

            std::destroy_at(ptr);
            allocator->free(m_mem);
        }
    }
};

template <typename F>
class Functor : public FunctorImpl<meta::GetCallableTypeInfo<F>::NoExcept, typename meta::GetCallableTypeInfo<F>::Result, typename meta::GetCallableTypeInfo<F>::ParametersList>
{
    using Base = FunctorImpl<
        meta::GetCallableTypeInfo<F>::NoExcept,
        typename meta::GetCallableTypeInfo<F>::Result,
        typename meta::GetCallableTypeInfo<F>::ParametersList>;

public:
    Functor() = default;

    Functor(F callable, IAllocator* allocator = nullptr) :
        Base(std::move(callable), allocator)
    {
    }
};

template <typename R, typename... P>
class Functor<R(P...)> : public FunctorImpl<false, R, TypeList<P...>>
{
    using Base = FunctorImpl<false, R, TypeList<P...>>;

public:
    Functor() = default;
    Functor(const Functor&) = delete;
    Functor(Functor&& other) :
        Base(std::move(static_cast<Base&>(other)))
    {
    }

    template <typename Callable>
    Functor(Callable callable, IAllocator* allocator = nullptr) :
        FunctorImpl<false, R, TypeList<P...>>(std::move(callable), allocator)
    {
    }

    Functor& operator=(Functor&& other) noexcept
    {
        this->reset();
        this->m_invocable = std::exchange(other.m_invocable, nullptr);
        this->m_allocator = std::exchange(other.m_allocator, nullptr);
        return *this;
    }

    Functor& operator=(std::nullptr_t) noexcept
    {
        this->reset();
        return *this;
    }
};

template <typename R, typename... P>
class Functor<R(P...) noexcept> : public FunctorImpl<true, R, TypeList<P...>>
{
    using This = Functor<R(P...) noexcept>;

public:
    Functor() = default;

    template <typename Callable>
    Functor(Callable callable, IAllocator* allocator = nullptr) :
        FunctorImpl<true, R, TypeList<P...>>(std::move(callable), allocator)
    {
    }

    This& operator=(This&& other) noexcept
    {
        this->reset();
        this->m_invocable = std::exchange(other.m_invocable, nullptr);
        this->m_allocator = std::exchange(other.m_allocator, nullptr);
        return *this;
    }

    This& operator=(std::nullptr_t) noexcept
    {
        this->reset();
        return *this;
    }
};

template <typename T>
Functor(T) -> Functor<T>;

template <typename F>
using IInvokable = IGenericInvokable<
    meta::GetCallableTypeInfo<F>::NoExcept,
    typename meta::GetCallableTypeInfo<F>::Result,
    typename meta::GetCallableTypeInfo<F>::ParametersList>;

template <typename F>
class InplaceFunctor : public Functor<F>
{
public:
    InplaceFunctor() = default;

    template <typename Callable>
    InplaceFunctor(Callable callable) :
        Functor<F>{std::move(callable), getRtStackAllocatorPtr()}
    {
    }
};

template <typename T>
InplaceFunctor(T) -> InplaceFunctor<T>;

}  // namespace my
