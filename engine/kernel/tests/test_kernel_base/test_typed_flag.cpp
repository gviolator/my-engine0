// #my_engine_source_file

#include "my/utils/typed_flag.h"


namespace my::test
{
    using namespace ::testing;

    enum class MyEnum : unsigned
    {
        Value1 = FlagValue(0),
        Value2 = FlagValue(1),
        Value1_2 = Value1 | Value2,
        Value3 = FlagValue(2),
        Value4 = FlagValue(3),
    };

    using MyEnumFlag = my::TypedFlag<MyEnum>;

    TEST(TestTypedFlag, EmptyByDefault)
    {
        MyEnumFlag flags;
        ASSERT_TRUE(flags.isEmpty());
    }

    TEST(TestTypedFlag, ConstructSingleEnumValue)
    {
        MyEnumFlag flags = MyEnum::Value1;
        ASSERT_FALSE(flags.isEmpty());
        ASSERT_TRUE(flags.has(MyEnum::Value1));
    }

    TEST(TestTypedFlag, ConstructInitializerList)
    {
        MyEnumFlag flags = {MyEnum::Value1, MyEnum::Value3};
        ASSERT_TRUE(flags.has(MyEnum::Value1));
        ASSERT_FALSE(flags.has(MyEnum::Value2));
        ASSERT_TRUE(flags.has(MyEnum::Value3));
    }

    TEST(TestTypedFlag, ConstructCopy)
    {
        const MyEnumFlag flags1 = {MyEnum::Value1, MyEnum::Value3};
        const auto flags2 = flags1;
        ASSERT_EQ(flags2, flags1);
    }

    TEST(TestTypedFlag, SetFlags)
    {
        MyEnumFlag flags;
        flags.set(MyEnum::Value1, MyEnum::Value3);

        ASSERT_TRUE(flags.has(MyEnum::Value1, MyEnum::Value3));
        ASSERT_FALSE(flags && MyEnum::Value2);

        flags |= MyEnum::Value2;
        ASSERT_TRUE(flags && MyEnum::Value2);
    }

    TEST(TestTypedFlag, UnsetFlag)
    {
        MyEnumFlag flags {MyEnum::Value1, MyEnum::Value2, MyEnum::Value3};
        flags -= MyEnum::Value2;

        ASSERT_TRUE(flags.has(MyEnum::Value1));
        ASSERT_FALSE(flags.has(MyEnum::Value2));
        ASSERT_TRUE(flags.has(MyEnum::Value3));
    }

    TEST(TestTypedFlag, UnsetMultipleFlags)
    {
        MyEnumFlag flags {MyEnum::Value1, MyEnum::Value2, MyEnum::Value3};
        flags -= {MyEnum::Value1, MyEnum::Value3};

        ASSERT_FALSE(flags.has(MyEnum::Value1));
        ASSERT_TRUE(flags.has(MyEnum::Value2));
        ASSERT_FALSE(flags.has(MyEnum::Value3));
    }

    TEST(TestTypedFlag, Equality)
    {
        {
            MyEnumFlag flags {MyEnum::Value1, MyEnum::Value3};
            flags.unset(MyEnum::Value3);
            ASSERT_THAT(flags, Eq(MyEnum::Value1));
        }

        {
            MyEnumFlag flags1 {MyEnum::Value1, MyEnum::Value3};
            MyEnumFlag flags2 {MyEnum::Value1, MyEnum::Value3};

            ASSERT_THAT(flags1, Eq(flags2));
        }
    }

    TEST(TestTypedFlag, HasAny)
    {
        MyEnumFlag flags {MyEnum::Value1, MyEnum::Value3};
        ASSERT_TRUE(flags.hasAny(MyEnum::Value1, MyEnum::Value2));
        ASSERT_FALSE(flags.has(MyEnum::Value1, MyEnum::Value2));
        ASSERT_FALSE(flags.hasAny(MyEnum::Value2, MyEnum::Value4));
        ASSERT_TRUE((flags + MyEnum::Value4).hasAny(MyEnum::Value4));
    }

    TEST(TestTypedFlag, IsConstexpr)
    {
        constexpr MyEnumFlag flag {MyEnum::Value1, MyEnum::Value2};

        static_assert(flag.has(MyEnum::Value2));
        static_assert(!flag.has(MyEnum::Value3));
    }

} // namespace my::test
