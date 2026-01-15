// #my_engine_source_file
#include "my/utils/result.h"

#include <span>
#include <string>
#include <string_view>


namespace my {

// int Base64Decode
struct Base64
{
    static int Decode(char c);

    static char Encode(uint8_t);

    static Result<int> VlqDecodeNext(std::string_view::iterator& current, const std::string_view::const_iterator end);

    static std::string VlqEncode(std::span<const int> values);
};

}  // namespace my
