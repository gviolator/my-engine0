// #my_engine_source_file

#pragma once

#include <array>
#include <exception>
#include <type_traits>
#include <typeinfo>
#include <utility>

#include "my/diag/check.h"
#include "my/diag/error.h"
#include "my/kernel/kernel_config.h"
#include "my/memory/mem_allocator.h"
#include "my/utils/scope_guard.h"
#include "my/utils/type_utility.h"

namespace my
{

    template <typename T = void>
    class Result;

    /**
     */
    template <typename T>
    class [[nodiscard]] Result
    {
        static_assert(!std::is_same_v<T, std::exception_ptr>, "std::exception_ptr is not acceptable type for Result<>");
        static_assert(!IsTemplateOf<Result, T>, "Result is not acceptable type for Result<>");

    public:
        using ValueType = T;

        Result()
        requires(std::is_default_constructible_v<T>)
            :
            m_value(std::in_place)
        {
        }

        Result(const Result& other)
        requires(std::is_copy_constructible_v<T>)
            :
            m_error(other.m_error),
            m_value(other.m_value)
        {
        }

        Result(Result&& other)
        requires(std::is_move_constructible_v<T>)
            :
            m_error(std::move(other.m_error)),
            m_value(std::move(other.m_value))
        {
        }

        template <typename U>
        requires(!std::is_same_v<T, U> && std::is_constructible_v<T, const U&>)
        Result(const Result<U>& other)
        {
            if (other.isError())
            {
                m_error = other.getError();
            }
            else
            {
                emplace(*other);
            }
        }

        template <typename U>
        requires(!std::is_same_v<T, U> && std::is_constructible_v<T, U &&>)
        Result(Result<U>&& other)
        {
            if (other.isError())
            {
                m_error = other.getError();
            }
            else
            {
                emplace(*std::move(other));
            }
        }

        template <typename U>
        requires(std::is_constructible_v<T, U &&>)
        Result(U&& value) noexcept
        {
            emplace(std::forward<U>(value));
        }
        /**
            construct in-place
        */
        template <typename... A>
        requires(std::is_constructible_v<T, A...>)
        Result(A&&... arg) :
            m_value(std::in_place, std::forward<A>(arg)...)
        {
        }

        /**
            construct error
        */
        template <ErrorConcept U>
        Result(Error::PtrType<U> error) :
            m_error(std::move(error))
        {
        }

        Result& operator=(const Result&) = default;

        Result& operator=(Result&& other) noexcept
        requires(std::is_move_assignable_v<T>)
        {
            m_error = std::move(other.m_error);
            m_value = std::move(other.m_value);

            return *this;
        }

        template <typename U>
        requires(!std::is_same_v<U, T> && std::is_assignable_v<T&, const U&>)
        Result& operator=(const Result<U>& other)
        {
            if (other.isError())
            {
                m_value.reset();
                m_error = other.getError();
            }
            else
            {
                assign(*other);
            }

            return *this;
        }

        /**
            move assign
        */
        template <typename U>
        requires(!std::is_same_v<U, T> && std::is_assignable_v<T&, U &&>)
        Result& operator=(Result<U>&& other)
        {
            if (other.isError())
            {
                m_value.reset();
                m_error = other.getError();
            }
            else
            {
                m_error.reset();
                assign(std::move(*other));
            }

            return *this;
        }

        template <typename U>
        requires(std::is_assignable_v<T&, U &&>)
        Result& operator=(U&& value)
        {
            m_error.reset();
            assign(std::forward<U>(value));

            return *this;
        }

        template <typename U>
        requires(IsError<U>)
        Result& operator=(Error::PtrType<U> error)
        {
            MY_DEBUG_CHECK(error);

            m_value.reset();
            m_error = std::move(error);
            return *this;
        }

        template <typename... A>
        requires(std::is_constructible_v<T, A...>)
        void emplace(A&&... args)
        {
            MY_DEBUG_CHECK(!m_error);
            m_value.emplace(std::forward<A>(args)...);
        }

        bool isError() const
        {
            return static_cast<bool>(m_error);
        }

        my::Error::Ptr getError() const
        {
            MY_DEBUG_CHECK(isError(), "Result<T> has no error");
            return m_error;
        }

        void ignore() const noexcept
        {
            MY_DEBUG_CHECK(!m_error, "Ignoring Result<T> that holds an error:{}", m_error->getMessage());
        }

        const T& operator*() const&
        {
            MY_DEBUG_CHECK(m_value, "Result<T> is valueless");
            return *m_value;
        }

        T& operator*() &
        {
            MY_DEBUG_CHECK(m_value, "Result<T> is valueless");
            return *m_value;
        }

        T&& operator*() &&
        {
            MY_DEBUG_CHECK(m_value, "Result<T> is valueless");
            return std::move(*m_value);
        }

        const T* operator->() const
        {
            MY_DEBUG_CHECK(m_value, "Result<T> is valueless");
            return &(*m_value);
        }

        T* operator->()
        {
            MY_DEBUG_CHECK(m_value, "Result<T> is valueless");
            return &(*m_value);
        }

        explicit operator bool() const
        {
            return m_value.has_value();
        }

    private:
        template <typename U>
        void assign(U&& value)
        {
            static_assert(std::is_assignable_v<T&, U>);
            m_error.reset();

            if (m_value)
            {
                *m_value = std::forward<U>(value);
            }
            else
            {
                m_value.emplace(std::forward<U>(value));
            }
        }

        Error::Ptr m_error = nullptr;
        std::optional<T> m_value;
    };

    /**
     */
    template <>
    class MY_KERNEL_EXPORT [[nodiscard]] Result<void>
    {
    public:
        using ValueType = void;

        Result() = default;
        Result(const Result<>&) = default;
        Result(Result&& other);

        template <typename U>
        requires(IsError<U>)
        Result(Error::PtrType<U> error) :
            m_error(std::move(error))
        {
            MY_DEBUG_CHECK(m_error);
        }

        Result<>& operator=(const Result&) = default;
        Result<>& operator=(Result&& other) noexcept
        {
            m_error = std::move(other.m_error);
            return *this;
        }

        template <typename U>
        requires(IsError<U>)
        Result<>& operator=(Error::PtrType<U> error)
        {
            MY_DEBUG_CHECK(error);
            m_error = std::move(error);
            return *this;
        }

        explicit operator bool() const;

        bool isError() const;

        bool isSuccess(Error::Ptr* = nullptr) const;

        my::Error::Ptr getError() const;

        void ignore() const noexcept;

    private:
        Error::Ptr m_error;
    };

    template <typename T>
    inline constexpr bool IsResult = my::IsTemplateOf<Result, std::decay_t<T>>;

    inline static Result<> ResultSuccess{};

}  // namespace my

#define CheckResult(expr)                                                                            \
    {                                                                                                \
        decltype(auto) exprResult = (expr);                                                          \
        static_assert(::my::IsTemplateOf<::my::Result, decltype(exprResult)>, "Expected Result<T>"); \
        if (exprResult.isError())                                                                    \
        {                                                                                            \
            return exprResult.getError();                                                            \
        }                                                                                            \
    }
