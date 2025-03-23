// #my_engine_source_header


#pragma once

#include <memory>
#include <type_traits>

#include "my/diag/check.h"
#include "my/meta/function_info.h"
#include "my/rtti/type_info.h"
#include "my/utils/type_utility.h"

namespace my
{

    template <bool, typename R, typename P>
    struct IGenericInvokable;

    template <bool NoExcept, typename Result, typename... Parameters>
    struct MY_ABSTRACT_TYPE IGenericInvokable<NoExcept, Result, TypeList<Parameters...>>
    {
        virtual ~IGenericInvokable() = default;

        virtual Result operator()(Parameters...) noexcept(NoExcept) = 0;
    };

    namespace kernel_detail
    {
        template <typename Callable, bool NoExcept, typename Result, typename Parameters>
        class GenericInvokableImpl;

        template <typename Callable, bool NoExcept, typename Result, typename... Parameters>
        class GenericInvokableImpl<Callable, NoExcept, Result, TypeList<Parameters...>> final : public IGenericInvokable<NoExcept, Result, TypeList<Parameters...>>
        {
        public:
            GenericInvokableImpl(Callable callable) :
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

        std::unique_ptr<InvokableItf> m_invocable;

        FunctorImpl() = default;

        FunctorImpl(std::nullptr_t) :
            FunctorImpl()
        {
        }

        template <typename Func>
        FunctorImpl(Func f)
        {
            using InvokableImpl = kernel_detail::GenericInvokableImpl<Func, NoExcept, R, TypeList<P...>>;

            static_assert(std::is_invocable_r_v<R, Func, P...>, "Functor has unacceptable parameters");
            static_assert(std::is_assignable_v<InvokableItf&, InvokableImpl&>);

            m_invocable = std::make_unique<InvokableImpl>(std::move(f));
        }

        FunctorImpl(FunctorImpl&&) = default;

        FunctorImpl(const FunctorImpl&) = delete;

        FunctorImpl& operator=(FunctorImpl&&) = default;

        FunctorImpl& operator=(std::nullptr_t)
        {
            m_invocable.reset();
            return *this;
        }

        R operator()(P... args) const noexcept(NoExcept)
        {
            MY_DEBUG_CHECK(m_invocable);
            return (*m_invocable)(std::forward<P>(args)...);
        }

        explicit operator bool() const noexcept
        {
            return static_cast<bool>(m_invocable);
        }
    };

    template <typename F>
    class Functor : public FunctorImpl<
                        meta::GetCallableTypeInfo<F>::NoExcept,
                        typename meta::GetCallableTypeInfo<F>::Result,
                        typename meta::GetCallableTypeInfo<F>::ParametersList>
    {
        using Base = FunctorImpl<
            meta::GetCallableTypeInfo<F>::NoExcept,
            typename meta::GetCallableTypeInfo<F>::Result,
            typename meta::GetCallableTypeInfo<F>::ParametersList>;

    public:
        Functor() = default;

        Functor(F callable) :
            Base(std::move(callable))
        {
        }
    };

    template <typename R, typename... P>
    class Functor<R(P...)> : public FunctorImpl<false, R, TypeList<P...>>
    {
    public:
        Functor() = default;

        template <typename Callable>
        Functor(Callable callable) :
            FunctorImpl<false, R, TypeList<P...>>(std::move(callable))
        {
        }
    };

    template <typename R, typename... P>
    class Functor<R(P...) noexcept> : public FunctorImpl<true, R, TypeList<P...>>
    {
    public:
        Functor() = default;

        template <typename Callable>
        Functor(Callable callable) :
            FunctorImpl<true, R, TypeList<P...>>(std::move(callable))
        {
        }
    };

    template <typename T>
    Functor(T) -> Functor<T>;

    template <typename F>
    using IInvokable = IGenericInvokable<
        meta::GetCallableTypeInfo<F>::NoExcept,
        typename meta::GetCallableTypeInfo<F>::Result,
        typename meta::GetCallableTypeInfo<F>::ParametersList>;

}  // namespace my
