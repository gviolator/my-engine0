// #my_engine_source_file

#include "my/memory/runtime_stack.h"
#include "my/utils/functor.h"

using namespace my::my_literals;

namespace my {

}  // namespace my

namespace my::test {
namespace {
inline void ignore([[maybe_unused]] const auto&)
{
}

struct MoveOnly
{
    MoveOnly() = default;

    MoveOnly(MoveOnly&&) = default;

    MoveOnly& operator=(MoveOnly&&) = default;
};

struct WithOperator
{
    unsigned operator()(float) const
    {
        return 0;
    }
};

}  // namespace

TEST(TestFunctor, EmptyByDefault)
{
    Functor<void()> f;
    ASSERT_FALSE(f);
}

TEST(TestFunctor, AutoDeclaration)
{
    Functor f = [](float x, float y) -> float
    {
        return x + y;
    };

    f(1.f, 2.f);
}

TEST(TestFunctor, FullSignatureDeclaration)
{
    Functor<float(float, float)> f = [](float x, float y) -> float
    {
        return x + y;
    };

    f(1.f, 2.f);
}

TEST(TestFunctor, MoveConstructible)
{
    Functor f = [value = MoveOnly{}]
    {
        ignore(value);
    };
    Functor<void()> f1 = std::move(f);

    ASSERT_FALSE(f);
    ASSERT_TRUE(f1);
}

TEST(TestFunctor, MoveAssignable)
{
    Functor f = [value = MoveOnly{}]()
    {
        ignore(value);
    };
    Functor<void()> f1;

    f1 = std::move(f);

    ASSERT_FALSE(f);
    ASSERT_TRUE(f1);
}

TEST(TestFunctor, CopyNotAllowed)
{
    using FunctorType = Functor<void()>;

    static_assert(std::is_move_constructible_v<FunctorType>);
    static_assert(std::is_move_assignable_v<FunctorType>);
    static_assert(!std::is_copy_constructible_v<FunctorType>);
    static_assert(!std::is_copy_assignable_v<FunctorType>);
}

TEST(TestFunctor, AcquireAnyCallable)
{
    const auto factory = []()
    {
        struct
        {
            int operator()(int, int)
            {
                return 0;
            }
        } callableObject;

        return callableObject;
    };

    [[maybe_unused]]
    Functor f = factory();

    static_assert(std::is_invocable_r_v<int, decltype(f), int, int>);
}

TEST(TestFunctor, AcquireAnyCallable2)
{
    [[maybe_unused]]
    Functor f = WithOperator{};
    static_assert(std::is_invocable_r_v<unsigned, decltype(f), float>);
}

TEST(TestFunctorInplace, EmptyByDefault)
{
    InplaceFunctor<void()> f;
    ASSERT_FALSE(f);
}

TEST(TestFunctorInplace, AutoDeclaration)
{
    InplaceFunctor f = [](float x, float y) -> float
    {
        return x + y;
    };

    f(1.f, 2.f);
}

TEST(TestFunctorInlplace, FullSignatureDeclaration)
{
    InplaceFunctor<float(float, float)> f = [](float x, float y) -> float
    {
        return x + y;
    };

    f(1.f, 2.f);
}

TEST(TestFunctorInlplace, MoveConstructible)
{
    InplaceFunctor f = [value = MoveOnly{}]
    {
        ignore(value);
    };
    InplaceFunctor<void()> f1 = std::move(f);

    ASSERT_FALSE(f);
    ASSERT_TRUE(f1);
}

TEST(TestFunctorInlplace, MoveAssignable)
{
    InplaceFunctor f = [value = MoveOnly{}]()
    {
        ignore(value);
    };
    InplaceFunctor<void()> f1;

    f1 = std::move(f);

    ASSERT_FALSE(f);
    ASSERT_TRUE(f1);
}

TEST(TestFunctorInlplace, CopyNotAllowed)
{
    using FunctorType = InplaceFunctor<void()>;

    static_assert(std::is_move_constructible_v<FunctorType>);
    static_assert(std::is_move_assignable_v<FunctorType>);
    static_assert(!std::is_copy_constructible_v<FunctorType>);
    static_assert(!std::is_copy_assignable_v<FunctorType>);
}

TEST(TestFunctorInlplace, AcquireAnyCallable)
{
    const auto factory = []()
    {
        struct
        {
            double operator()(int, int)
            {
                return 0.;
            }
        } callableObject;

        return callableObject;
    };

    [[maybe_unused]]
    InplaceFunctor f = factory();

    static_assert(std::is_invocable_r_v<double, decltype(f), int, int>);
}

}  // namespace my::test
