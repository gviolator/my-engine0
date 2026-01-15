// #my_engine_source_file

#include "my/utils/string_utils.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

namespace my::test {

TEST(StringUtils_Split, EmptyString)
{
    const char* kEmptyStr = "";
    const strings::SplitSequence seq = strings::Split(kEmptyStr, ";");

    const auto count = std::distance(seq.begin(), seq.end());
    ASSERT_EQ(count, 0);
}

TEST(StringUtils_Split, SingleElement)
{
    const char* kStr = "first";
    const strings::SplitSequence seq = strings::Split(kStr, ";");
    auto iter = seq.begin();
    ASSERT_NE(iter, seq.end());
    EXPECT_EQ(*iter, "first");
    EXPECT_EQ(++iter, seq.end());
}

TEST(StringUtils_Split, MultiElements)
{
    const char* kStr = "first;second|third,fourth";
    const char* kSep = ";|,";
    const strings::SplitSequence seq = strings::Split(kStr, kSep);

    std::vector<std::string> elements;
    std::transform(seq.begin(), seq.end(), std::back_inserter(elements), [](auto sv)
    {
        return std::string{sv};
    });

    ASSERT_EQ(elements.size(), 4);
    EXPECT_EQ(elements[0], "first");
    EXPECT_EQ(elements[1], "second");
    EXPECT_EQ(elements[2], "third");
    EXPECT_EQ(elements[3], "fourth");
}

TEST(StringUtils_Split, NoSkipEmptyElements)
{
    const char* kStr = ",A,,B,";
    const std::vector<std::string_view> expectedElements = {"", "A", "", "B", ""};

    const strings::SplitSequence seq = strings::Split(kStr, ",");
    std::vector<std::string_view> elements;
    std::copy(seq.begin(), seq.end(), std::back_inserter(elements));

    ASSERT_EQ(elements.size(), 5);
    EXPECT_EQ(elements, expectedElements);
}
}  // namespace my::test
