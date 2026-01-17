// #my_engine_source_file
#include "base_lua_test.h"
// #include "lua_toolkit/lua_utils.h"
#include "my/dispatch/closure_value.h"
#include "my/memory/mem_base.h"
#include "my/memory/runtime_stack.h"

using namespace my::my_literals;
using namespace my::lua;

namespace my::test
{
    class TestReferencibleValue : public testing::TestWithParam<ValueKeepMode>,
                                  public BaseLuaTest
    {
    private:
        rtstack_init(1_Mb);
    };

    TEST_P(TestReferencibleValue, RefToTable)
    {
        executeProgram(R"--(
            local tbl = {}
            tbl.str = 'my_table'
            return tbl
        )--");

        ASSERT_NO_FATAL_FAILURE();

        const ValueKeepMode keepMode = GetParam();
        const ValueKeeperGuard keepGuard{getLua(), keepMode};

        assert_lstack_unchanged(getLua());

        const int top0 = lua_gettop(getLua());

        auto [value, _] = makeValueFromLuaStack(getLua(), -1);

        ASSERT_TRUE(value);
        ASSERT_TRUE(value->is<ReadonlyDictionary>());

        {
            Ptr<ReadonlyDictionary> dict = value;
            ASSERT_TRUE(dict->containsKey("str"));

            std::string str = dict->getValue("str")->as<StringValue&>().getString();
            ASSERT_EQ(str, "my_table");
        }

        // returning value type must not involve stack changes.
        ASSERT_EQ(top0, lua_gettop(getLua()));
    };

    TEST_P(TestReferencibleValue, RefToFunction)
    {
        executeProgram(R"--(
            return function(a, b)
                return a + b
            end
        )--");

        ASSERT_NO_FATAL_FAILURE();

        const ValueKeepMode keepMode = GetParam();
        const ValueKeeperGuard keepGuard{getLua(), keepMode};

        auto [value, _] = makeValueFromLuaStack(getLua(), -1);

        ASSERT_TRUE(value);
        ASSERT_TRUE(value->is<ClosureValue>());

        Result<Ptr<>> result = EXPR_Block
        {
            rtstack_scope;
            Ptr<ClosureValue> closure = value;
            DispatchArguments args = {
                makeValueCopy(10, GetRtStackAllocatorPtr()),
                makeValueCopy(17, GetRtStackAllocatorPtr())};

            // force to keep result value on heap
            // do not
            const ValueKeeperGuard keepGuard{getLua(), ValueKeepMode::AsReference};
            return closure->invoke(std::move(args));
        };
        ASSERT_TRUE(result);
        ASSERT_TRUE((*result)->is<IntegerValue>());
        const int val = (*result)->as<const IntegerValue&>().get<int>();
        ASSERT_EQ(val, 27);
    }

    TEST(TestPMR, Test1)
    {
        rtstack_init(2_Mb);

        using Str = std::basic_string<char, std::char_traits<char>, std::pmr::polymorphic_allocator<char>> ;

        std::vector<Str, std::pmr::polymorphic_allocator<Str>> v {GetRtStackAllocator().GetMemoryResource()};
        v.emplace_back("Test1");
        v.emplace_back("Test2");
        //std::vector<Str, std::pmr::polymorphic_allocator<Str>> v2 {GetRtStackAllocator().GetMemoryResource()};


        std::vector<Str, std::pmr::polymorphic_allocator<Str>> v2 {getDefaultAllocator().GetMemoryResource()};//= std::move(v);
        v2 = std::move(v);
        
        v2.reserve(5);

        v2.push_back("SSS_");
        v2.push_back("SSS_");
        v2.push_back("SSS_");
        v2.push_back("SSS_");
    

        

        // v2 = std::move(v);
    }

    INSTANTIATE_TEST_SUITE_P(
        Default,
        TestReferencibleValue,
        testing::Values(ValueKeepMode::AsReference, ValueKeepMode::OnStack));

}  // namespace my::test
