// #my_engine_source_file

#include "my/utils/base64.h"
#include <tuple>
#include <vector>

namespace {

using namespace my;


using VlqEntry = std::tuple<std::string_view, std::vector<int>>;

const std::vector<VlqEntry> GetVlqTestData()
{
    // Test data sets: 2, 3, 4, and 5 numbers per set
    // Each set contains:
    // - Numbers without continuation bit (single char)
    // - Numbers with continuation bit (multi-char)
    // - Positive and negative values
    // - Mixed values

    const std::vector<VlqEntry> entries = {
        // === Set with 2 numbers ===
        {    "AC",          {0, 1}},
        {    "EG",          {2, 3}},
        {   "oAC",          {4, 1}},
        {    "IC",          {4, 1}},

        // === Set with 3 numbers ===
        {   "ACE",       {0, 1, 2}},
        {   "GIK",       {3, 4, 5}},
        {  "oACD",      {4, 1, -1}},

        // === Set with 4 numbers ===
        {  "ACEG",    {0, 1, 2, 3}},
        {  "IKMO",    {4, 5, 6, 7}},
        {"cBoA+E",  {14, 0, 4, 79}},
        { "oACEG",    {4, 1, 2, 3}},
        {"AhBwBC", {0, -16, 24, 1}},

        // === Set with 5 numbers ===
        { "ACEIK", {0, 1, 2, 4, 5}},
        { "EGMOK", {2, 3, 6, 7, 5}},
        {"ACEGoA", {0, 1, 2, 3, 4}},
        {"oACEIK", {4, 1, 2, 4, 5}}
    };

    return entries;
}

void DecodeVlqToVec(std::string_view vlq, std::vector<int>& result)
{
    result.clear();

    Result<int> value;
    for (auto iter = vlq.begin(); iter != vlq.end() && (value = my::Base64::VlqDecodeNext(iter, vlq.end()));)
    {
        result.push_back(*value);
    }
}

}  // namespace

namespace my::test {

TEST(TestBase64VLQ, Decode)
{
    std::vector<int> values;
    for (const auto& [vlqStr, expected] : GetVlqTestData())
    {
        DecodeVlqToVec(vlqStr, values);
        EXPECT_EQ(expected, values);
    }
}

TEST(TestBase64VLQ, Encode)
{
    std::vector<int> values;
    for (const auto& [vlqStr, expected] : GetVlqTestData())
    {
        // NOTE: same sequence can be encoded by difference way:
        // must compare only values
        const std::string codedString = Base64::VlqEncode(expected);
        DecodeVlqToVec(codedString, values);
        EXPECT_EQ(expected, values);
    }
}

}  // namespace my::test
