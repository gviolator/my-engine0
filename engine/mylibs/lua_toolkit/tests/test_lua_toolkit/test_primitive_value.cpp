// #my_engine_source_file
#include "base_lua_test.h"
#include "lua_toolkit/lua_headers.h"
#include "lua_toolkit/lua_interop.h"
#include "lua_toolkit/lua_utils.h"
#include "my/memory/runtime_stack.h"
#include "my/memory/mem_base.h"

using namespace testing;
using namespace my::my_literals;

namespace my::test
{
    class TestPrimitiveValue : public testing::Test,
                               public BaseLuaTest
    {
    protected:
        template <typename T, typename U>
        void checkValue(const U& expected, int stackIndex = -1) const
        {
            using ValueType = std::decay_t<U>;

            assert_lstack_unchanged(getLua());

            const lua::ValueKeeperGuard valueKeepGuard{getLua(), lua::ValueKeepMode::OnStack};
            const auto [valueWrapper, _ ] = lua::makeValueFromLuaStack(getLua(), stackIndex);

            ASSERT_TRUE(valueWrapper);
            ASSERT_TRUE(valueWrapper->is<T>());

            Result<ValueType> value = runtimeValueCast<ValueType>(RuntimeValuePtr{valueWrapper});
            ASSERT_TRUE(value);
            ASSERT_EQ(*value, expected);
        }

    private:
        rtstack_init(1_Mb);
    };

    TEST_F(TestPrimitiveValue, StringValue)
    {
        ASSERT_NO_FATAL_FAILURE(executeProgram("return 'text_from_lua'"));

        const std::string ExpectedValue = "text_from_lua";
        ASSERT_NO_FATAL_FAILURE(checkValue<StringValue>(ExpectedValue));
    }

    TEST_F(TestPrimitiveValue, IntegerValue)
    {
        ASSERT_NO_FATAL_FAILURE(executeProgram("return 777"));

        ASSERT_NO_FATAL_FAILURE(checkValue<IntegerValue>(777));
        ASSERT_NO_FATAL_FAILURE(checkValue<FloatValue>(777.0f));
    }

    TEST_F(TestPrimitiveValue, FloatValue)
    {
        ASSERT_NO_FATAL_FAILURE(executeProgram("return 99.0"));

        // TODO: replace by floating point comparison
        ASSERT_NO_FATAL_FAILURE(checkValue<FloatValue>(99.0f));
        ASSERT_NO_FATAL_FAILURE(checkValue<IntegerValue>(99));
    }

    TEST_F(TestPrimitiveValue, BooleanValue)
    {
        ASSERT_NO_FATAL_FAILURE(executeProgram("return false"));
        ASSERT_NO_FATAL_FAILURE(checkValue<BooleanValue>(false));

        ASSERT_NO_FATAL_FAILURE(executeProgram("return true"));
        ASSERT_NO_FATAL_FAILURE(checkValue<BooleanValue>(true));
    }

    TEST_F(TestPrimitiveValue, NilValue)
    {
        ASSERT_NO_FATAL_FAILURE(executeProgram("return nil"));
        assert_lstack_unchanged(getLua());

        auto [value, mode] = lua::makeValueFromLuaStack(getLua(), -1);
        const OptionalValue* opt = value->as<const OptionalValue*>();
        ASSERT_TRUE(opt != nullptr);
        ASSERT_FALSE(opt->hasValue());
    }

#if 0
    TEST_F(TestStackValue, TableValue)
    {
        executeProgram(R"--(
            function makeValue()
                local tbl = {}
                tbl.b = true
                tbl.f = 77

                local obj = {}
                obj.x = 101
                obj.y = 202
                obj.str = 'from_lua'
                obj.tbl = tbl

                return obj
            end
        )--");

        ASSERT_TRUE(call("makeValue"));

        RuntimeValuePtr value = lua::makeValueFromLuaStack(getLua(), -1, false);
        ASSERT_TRUE(value);
        ASSERT_TRUE(value->is<ReadonlyDictionary>());

        Ptr<ReadonlyDictionary> dict = value;

        ASSERT_TRUE(dict->containsKey("x"));
        ASSERT_TRUE(dict->containsKey("y"));
        ASSERT_TRUE(dict->containsKey("str"));
        ASSERT_TRUE(dict->containsKey("tbl"));
        ASSERT_FALSE(dict->containsKey("no_field"));

        EXPECT_TRUE(dict->getValue("x")->is<IntegerValue>());
        EXPECT_TRUE(dict->getValue("y")->is<IntegerValue>());
        EXPECT_TRUE(dict->getValue("str")->is<StringValue>());
        EXPECT_TRUE(dict->getValue("tbl")->is<ReadonlyDictionary>());
    }
#endif
}  // namespace my::test
