// #my_engine_source_file
#include "my/diag/assert.h"
#include "my/utils/base64.h"

#include <array>

namespace my {

int Base64::Decode(char c)
{
    static const auto table = []
    {
        std::array<int8_t, 256> t;
        t.fill(-1);

        for (uint8_t ch = 'A'; ch <= 'Z'; ++ch)
        {
            t[ch] = ch - 'A';
        }

        constexpr uint8_t ofs0 = 'Z' - 'A' + 1;
        for (uint8_t ch = 'a'; ch <= 'z'; ++ch)
        {
            t[ch] = ch - 'a' + ofs0;
        }

        constexpr uint8_t ofs1 = ofs0 + 'z' - 'a' + 1;
        for (uint8_t ch = '0'; ch <= '9'; ++ch)
        {
            t[ch] = ch - '0' + ofs1;
        }

        t[static_cast<uint8_t>('+')] = 62;
        t[static_cast<uint8_t>('/')] = 63;
        return t;
    }();

    return table[static_cast<unsigned char>(c)];
}

char Base64::Encode(uint8_t digit)
{
    // digit should be in [0..63]
    static constexpr char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    MY_DEBUG_ASSERT(digit >= 0 && digit <= 63);
    return table[digit & 63];
}

Result<int> Base64::VlqDecodeNext(std::string_view::iterator& iter, const std::string_view::const_iterator end)
{
    // Result<int> result {0};
    //  Use an unsigned accumulator to avoid UB on left shifts,
    //  and be explicit about the 5-bit groups.
    unsigned int value = 0;
    int shift = 0;

    while (true)
    {
        if (iter == end)
        {
            return MakeError("End of sequence");
        }

        const char chr = *iter++;
        const int digit = Decode(chr);
        if (digit < 0)
        {
            // throw std::invalid_argument("");
            return MakeError("Invalid base64 character ({}) in VLQ sequence", chr);
        }

        // digit is in [0..63]
        const bool continuation = (digit & 32) != 0;  // continuation bit (6th bit)
        value |= static_cast<unsigned int>(digit & 31u) << shift;
        shift += 5;
        if (!continuation)
        {
            break;
        }
        // defensive: avoid undefined behavior on extremely long sequences
        if (shift >= 32)
        {
            break;
        }
    }

    // Least significant bit is sign
    const bool isNegative = (value & 1u) != 0;
    unsigned int magnitude = value >> 1u;
    int result = static_cast<int>(magnitude);
    if (isNegative)
    {
        result = -result;
    }

    return result;
}

std::string Base64::VlqEncode(std::span<const int> values)
{
    std::string result;

    for (int val : values)
    {
        // Encode value: LSB is sign, rest is magnitude
        unsigned int encoded;
        if (val < 0)
        {
            encoded = (static_cast<unsigned int>(-val) << 1) | 1;  // negative: sign bit = 1
        }
        else
        {
            encoded = static_cast<unsigned int>(val) << 1;  // positive: sign bit = 0
        }

        // Encode in base64 with continuation bits
        while (true)
        {
            int digit = encoded & 31;  // take 5 bits
            encoded >>= 5;

            // Set continuation bit if more data remains
            if (encoded > 0)
            {
                digit |= 32;  // set continuation bit (bit 5)
            }

            result += Encode(static_cast<uint8_t>(digit));

            if (encoded == 0)
            {
                break;
            }
        }
    }

    return result;
}

}  // namespace my