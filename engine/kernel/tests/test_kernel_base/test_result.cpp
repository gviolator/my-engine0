// #my_engine_source_file
#if 0

#include "my/diag/error.h"
#include "my/utils/functor.h"
#include "my/utils/result.h"
#include "my/utils/scope_guard.h"

namespace my::test
{
    using namespace testing;

    class CustomErrorImpl;

    struct MY_ABSTRACT_TYPE ICustomError : Error
    {
        MY_ABSTRACT_ERROR(my::test::ICustomError, Error)
    };

    class CustomError final : public DefaultError<ICustomError>
    {
        using ErrorBase = DefaultError<ICustomError>;
        MY_ERROR(my::test::CustomErrorImpl, ErrorBase)

    public:
        CustomError(const char* message = "sample failure") :
            ErrorBase(diag::SourceInfo{"", ""}, message)
        {
        }
    };

    struct Destructible
    {
        Functor<void()> onDestruct;

        template <typename F,
                  std::enable_if_t<std::is_invocable_r_v<void, F>, int> = 0>
        Destructible(F f) :
            onDestruct(std::move(f))
        {
        }

        Destructible(Destructible&&) = default;

        ~Destructible()
        {
            if (onDestruct)
            {
                onDestruct();
            }
        }
    };

    struct MyValue
    {
        bool defaultConstructed = false;
        bool copyConstructed = false;
        bool moveConstructed = false;
        bool compatConstructed = false;
        bool copyAssigned = false;
        bool moveAssigned = false;
        bool compatAssigned = false;
        bool isMoved = false;

        MyValue() :
            defaultConstructed(true)
        {
        }

        MyValue(std::string_view) :
            compatConstructed(true)
        {
        }

        MyValue(const MyValue&) :
            copyConstructed(true)
        {
        }

        MyValue(MyValue&& other) noexcept :
            moveConstructed(true)
        {
            other.isMoved = true;
        }

        MyValue& operator=(const MyValue&)
        {
            copyAssigned = true;
            return *this;
        }

        MyValue& operator=(MyValue&& other) noexcept
        {
            moveAssigned = true;
            other.isMoved = true;
            return *this;
        }

        MyValue& operator=(std::string_view)
        {
            compatAssigned = true;
            return *this;
        }
    };

    struct MoveOnly
    {
        MoveOnly() = default;
        MoveOnly(MoveOnly&&) noexcept
        {
        }

        MoveOnly& operator=(MoveOnly&&) = default;
    };

    /**
     */
    TEST(TestResult, ResultVoidConstructSuccess)
    {
        Result<> result;
        EXPECT_TRUE(result);
        EXPECT_FALSE(result.isError());

        Result<> result2 = my::kResultSuccess;
        EXPECT_TRUE(result2);
        EXPECT_FALSE(result2.isError());
    }

    TEST(TestResult, ResultVoidConstructError)
    {
        Result<> result = MakeError("test");
        EXPECT_FALSE(result);
        EXPECT_FALSE(result);
        EXPECT_TRUE(result.isError());
    }

    /**
     */
    TEST(TestResult, ConstructInplace)
    {
        using Value = std::tuple<int, std::string>;
        Result<Value> res(10, "text");

        auto [i, s] = *res;
        ASSERT_THAT(i, Eq(10));
        ASSERT_THAT(s, Eq("text"));

        Result<MyValue> resValue;
        resValue.emplace();
        EXPECT_TRUE(resValue->defaultConstructed);
    }

    /**
     */
    TEST(TestResult, ConstructValueCopy)
    {
        const MyValue value;
        Result<MyValue> res = value;

        EXPECT_TRUE(res);
        EXPECT_TRUE(res->copyConstructed);
    }

    /**
     */
    TEST(TestResult, ConstructValueMove)
    {
        Result<MyValue> res = MyValue{};

        EXPECT_TRUE(res);
        EXPECT_TRUE(res->moveConstructed);
    }

    /**
     */
    TEST(TestResult, ConstructResultCopy)
    {
        Result<MyValue> src;
        src.emplace();

        Result<MyValue> res = src;

        EXPECT_TRUE(res);
        EXPECT_TRUE(res->copyConstructed);
    }

    /**
     */
    TEST(TestResult, ConstructResultMove)
    {
        Result<MyValue> src;
        src.emplace();

        Result<MyValue> res = std::move(src);

        EXPECT_TRUE(res);
        EXPECT_TRUE(res->moveConstructed);
    }

    TEST(TestResult, ConstructResultMoveError)
    {
        Result<MyValue> src = MakeError("Error");
        Result<MyValue> res = std::move(src);

        EXPECT_TRUE(res.isError());
    }

    TEST(TestResult, ConstructResultCompatible)
    {
        {
            Result<float> src = 10;
            Result<double> dst = src;

            EXPECT_TRUE(dst);
        }

        {
            Result<std::string> src("test");
            Result<MyValue> dst = src;

            EXPECT_TRUE(dst->compatConstructed);
        }

        {
            Result<std::string> src("test");
            Result<MyValue> dst = std::move(src);

            EXPECT_TRUE(dst->compatConstructed);
        }
    }

    /**
     */
    TEST(TestResult, AssignValueCopy)
    {
        const MyValue value;
        Result<MyValue> res;
        res.emplace();
        res = value;

        EXPECT_TRUE(res->defaultConstructed);
        EXPECT_TRUE(res->copyAssigned);
    }

    /**
     */
    TEST(TestResult, AssignValueMove)
    {
        MyValue value;
        Result<MyValue> res;
        res.emplace();
        res = std::move(value);

        EXPECT_TRUE(res->defaultConstructed);
        EXPECT_TRUE(res->moveAssigned);
        EXPECT_TRUE(value.isMoved);
    }

    TEST(TestResult, AssignResultCompatible)
    {
        {
            Result<float> src = 10;
            Result<double> dst;
            dst = src;

            EXPECT_TRUE(dst);
        }

        {
            Result<std::string> src("test");
            Result<MyValue> dst;
            dst.emplace();
            dst = src;

            EXPECT_TRUE(dst->compatAssigned);
        }

        {
            Result<std::string> src("test");
            Result<MyValue> dst;
            dst.emplace();
            dst = std::move(src);

            EXPECT_TRUE(dst);
            EXPECT_TRUE(dst->compatAssigned);
        }
    }

    /**
     */
    TEST(TestResult, ConstructError)
    {
        {
            Result<MyValue> res = MakeErrorT(CustomError)("error text");
            EXPECT_FALSE(res);
            ASSERT_TRUE(res.isError());
            ASSERT_THAT(res.getError(), NotNull());
        }

        {
            Result<> res = MakeErrorT(CustomError)();
            EXPECT_FALSE(res);
            ASSERT_TRUE(res.isError());
            ASSERT_THAT(res.getError(), NotNull());
        }
    }

    /**
     */
    TEST(TestResult, AssignError)
    {
        {
            Result<MyValue> res;
            // res.emplace();
            res = MakeErrorT(CustomError)();

            EXPECT_FALSE(res);
            ASSERT_TRUE(res.isError());
            ASSERT_THAT(res.getError(), NotNull());
        }

        {
            Result<> res;
            res = MakeErrorT(CustomError)();

            EXPECT_FALSE(res);
            ASSERT_TRUE(res.isError());
            ASSERT_THAT(res.getError(), NotNull());
        }
    }

    TEST(TestResult, ValueDestructed)
    {
        {
            bool destructed = false;

            {
                Result<Destructible> value{
                    [&]
                {
                    destructed = true;
                }};
            }

            EXPECT_TRUE(destructed);
        }

        {
            bool destructed = false;

            {
                Result<Destructible> value([&]
                {
                    destructed = true;
                });
            }

            EXPECT_TRUE(destructed);
        }

        {
            bool destructed = false;

            Result<Destructible> value{
                [&]
            {
                destructed = true;
            }};
            value = MakeError("fail");
            EXPECT_TRUE(destructed);
        }
    }

    TEST(TestResult, ResultTraits)
    {
        static_assert(std::is_copy_constructible_v<MoveOnly> == std::is_copy_constructible_v<Result<MoveOnly>>);
        static_assert(std::is_move_constructible_v<MoveOnly> == std::is_move_constructible_v<Result<MoveOnly>>);
        static_assert(std::is_copy_assignable_v<MoveOnly> == std::is_copy_assignable_v<Result<MoveOnly>>);
        static_assert(std::is_move_assignable_v<MoveOnly> == std::is_move_assignable_v<Result<MoveOnly>>);

        static_assert(std::is_copy_constructible_v<std::string> == std::is_copy_constructible_v<Result<std::string>>);
        static_assert(std::is_move_constructible_v<std::string> == std::is_move_constructible_v<Result<std::string>>);
        static_assert(std::is_copy_assignable_v<std::string> == std::is_copy_assignable_v<Result<std::string>>);
        static_assert(std::is_move_assignable_v<std::string> == std::is_move_assignable_v<Result<std::string>>);
    }

    TEST(TestResult, MoveOnly)
    {
        static_assert(!std::is_copy_constructible_v<Result<MoveOnly>>);
        static_assert(!std::is_copy_assignable_v<Result<MoveOnly>>);

        {
            Result<MoveOnly> res;
            Result<MoveOnly> resMoveConstructed = std::move(res);
            EXPECT_TRUE(resMoveConstructed);
        }

        {
            Result<MoveOnly> res;
            Result<MoveOnly> resMoveAssigned;
            resMoveAssigned = std::move(res);
            EXPECT_TRUE(resMoveAssigned);
        }

        {
            Result<MoveOnly> resMoveConstructed = MoveOnly{};
            EXPECT_TRUE(resMoveConstructed);
        }

        {
            Result<MoveOnly> resMoveAssigned;
            resMoveAssigned = MoveOnly{};
            EXPECT_TRUE(resMoveAssigned);
        }
    }

    TEST(TestResult, IsSuccess)
    {
        const auto GetResult = [](size_t i, bool* called = nullptr) -> Result<>
        {
            if (called)
            {
                *called = true;
            }
            if (i >= 3)
            {
                return MakeError("Too big");
            }

            return {};
        };

        const bool expectedTrue = GetResult(0).isSuccess() && GetResult(1).isSuccess() && GetResult(2).isSuccess();
        EXPECT_TRUE(expectedTrue);

        bool called0 = false;
        bool called1 = false;
        bool called2 = false;
        bool called3 = false;
        bool called4 = false;
        bool called5 = false;

        ErrorPtr error;

        const bool expectedFalse =
            GetResult(0, &called0).isSuccess(&error) &&
            GetResult(1, &called1).isSuccess(&error) &&
            GetResult(2, &called2).isSuccess(&error) &&
            GetResult(3, &called3).isSuccess(&error) &&
            GetResult(4, &called4).isSuccess(&error) &&
            GetResult(5, &called5).isSuccess(&error);

        EXPECT_TRUE(error);
        ASSERT_THAT(error->getMessage(), Eq("Too big"));
        EXPECT_FALSE(expectedFalse);
        EXPECT_TRUE(called0);
        EXPECT_TRUE(called1);
        EXPECT_TRUE(called2);
        EXPECT_TRUE(called3);
        EXPECT_FALSE(called4);
        EXPECT_FALSE(called5);
    }

    TEST(TestResult, CheckResult)
    {
        const auto useResult = [](const auto& factory, bool& executeAfterCheck) -> Result<unsigned>
        {
            executeAfterCheck = false;
            CheckResult(factory());
            executeAfterCheck = true;

            return 1;
        };

        {
            const auto makeSuccess = []() -> Result<>
            {
                return {};
            };

            bool executed = false;

            auto result = useResult(makeSuccess, executed);
            EXPECT_TRUE(result);
            EXPECT_TRUE(executed);
        }

        {
            const auto makeFailure = []() -> Result<std::string>
            {
                return MakeError("Test");
            };

            bool executed = false;

            auto result = useResult(makeFailure, executed);
            EXPECT_FALSE(result);
            EXPECT_TRUE(result.isError());
            EXPECT_FALSE(executed);
        }
    }

}  // namespace my::test

#endif
