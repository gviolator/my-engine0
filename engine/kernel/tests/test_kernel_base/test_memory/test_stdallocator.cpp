// #my_engine_source_file

#include "my/memory/allocator.h"

namespace my::test
{
#if 0
    TEST(TestStdAllocator, Test1)
    {
        using Str = std::basic_string<char, std::char_traits<char>, DefaultStdAllocator<char>>;

        std::vector<Str, DefaultStdAllocator<Str>> vec1;

        vec1.emplace_back("test1");
        vec1.emplace_back("test2");

        std::vector<Str, DefaultStdAllocator<Str>> vec2 = std::move(vec1);
        vec1.clear();

        ASSERT_EQ(vec2[0], "test1");
        ASSERT_EQ(vec2[1], "test2");
    }
#endif
}  // namespace my::test
