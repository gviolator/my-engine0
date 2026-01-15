#include "my/io/file_system.h"
#include "my/serialization/json.h"
#include "my/serialization/json_utils.h"
#include "my/serialization/runtime_value_builder.h"
#include "my/utils/string_utils.h"

#include <array>
#include <stdexcept>

namespace my::test {

std::vector<char> ReadSourceMapFile()
{
    io::StreamPtr stream = io::createNativeFileStream("c:\\proj\\my-engine0\\workshop\\ts_sample1\\content\\scripts\\out.lua\\script1.lua.map", io::AccessMode::Read, io::OpenFileMode::OpenExisting);

    stream->setPosition(io::OffsetOrigin::End, 0);
    const size_t size = stream->getPosition();
    stream->setPosition(io::OffsetOrigin::Begin, 0);

    std::vector<char> buffer;
    buffer.resize(size);

    stream->read(reinterpret_cast<std::byte*>(buffer.data()), buffer.size()).ignore();
    return buffer;
}


// Базовая таблица Base64
int base64_decode(char c) {
    // table built once; -1 marks invalid character
    static const auto table = []{
        std::array<int8_t, 256> t;
        t.fill(-1);
        for (char ch = 'A'; ch <= 'Z'; ++ch) t[static_cast<uint8_t>(ch)] = ch - 'A';
        for (char ch = 'a'; ch <= 'z'; ++ch) t[static_cast<uint8_t>(ch)] = ch - 'a' + 26;
        for (char ch = '0'; ch <= '9'; ++ch) t[static_cast<uint8_t>(ch)] = ch - '0' + 52;
        t[static_cast<uint8_t>('+')] = 62;
        t[static_cast<uint8_t>('/')] = 63;
        return t;
    }();

    return table[static_cast<unsigned char>(c)];
}

// Декодирование одного числа VLQ из строки
int decode_vlq(std::string_view::const_iterator& it) {
    // Use an unsigned accumulator to avoid UB on left shifts,
    // and be explicit about the 5-bit groups.
    unsigned int value = 0;
    int shift = 0;

    while (true) {
        int digit = base64_decode(*it++);
        if (digit < 0) {
            throw std::invalid_argument("Invalid base64 character in VLQ sequence");
        }
        // digit is in [0..63]
        bool continuation = (digit & 32) != 0; // continuation bit (6th bit)
        value |= static_cast<unsigned int>(digit & 31u) << shift;
        shift += 5;
        if (!continuation) break;
        // defensive: avoid undefined behavior on extremely long sequences
        if (shift >= 32) break;
    }

    // Least significant bit is sign
    bool is_negative = (value & 1u) != 0;
    unsigned int magnitude = value >> 1u;
    int result = static_cast<int>(magnitude);
    return is_negative ? -result : result;
}

int base64_decode2(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return 0;
}

// Декодирование одного числа VLQ из строки
int decode_vlq2(std::string_view::const_iterator& it) {
    int result = 0;
    int shift = 0;
    bool continuation;
    do {
        int digit = base64_decode2(*it++);
        continuation = (digit & 32) != 0; // 5-й бит указывает, есть ли продолжение
        result += (digit & 31) << shift;  // первые 5 бит - данные
        shift += 5;
    } while (continuation);

    // Последний бит результата - это знак (Sign bit)
    bool is_negative = (result & 1) != 0;
    result >>= 1;
    return is_negative ? -result : result;
}

struct SourceMapData
{
    unsigned version;
    std::vector<std::string> sources;
    std::vector<std::string> names;
    std::string mappings;

    MY_CLASS_FIELDS(
        CLASS_FIELD(version),
        CLASS_FIELD(sources),
        CLASS_FIELD(names),
        CLASS_FIELD(mappings))
};

struct SourceColumn
{
    unsigned dstColumn  = 0;
    unsigned srcIndex = 0;
    unsigned srcLine = 0;
    unsigned srcColumn = 0;
    int nameIndex = 0;

};

struct SourceLine
{
    std::vector<SourceColumn> columns;
};

TEST(TestSourceMap, Parse)
{
    constexpr size_t kDstColumn_Idx = 0;
    constexpr size_t kSourceId_Idx = 1;
    constexpr size_t kSourceLine_Idx = 2;
    constexpr size_t kSourceColumn_Idx = 3;
    constexpr size_t kNameIdx_Idx = 4;

    const std::vector<char> buffer = ReadSourceMapFile();
    const SourceMapData data = *serialization::JsonUtils::parse<SourceMapData>({buffer.data(), buffer.size()});

    std::map<unsigned, std::vector<SourceColumn>> mappingData;

    //for (auto x : strings::Split(",", ","))
    //{
    //    std::cout << std::format("* [{}]\n", x);
    //}



    SourceColumn accum;
    unsigned lineNo = 0;
    for (std::string_view line : strings::Split(data.mappings, ";"))
    {
        scope_on_leave
        {
            ++lineNo;
        };

        if (line.empty())
        {
            continue;
        }

        accum.dstColumn = 0;

        auto [iter, ok] = mappingData.emplace(lineNo, std::vector<SourceColumn>{});
        auto& columns = iter->second;

        for (std::string_view column : strings::Split(line, ","))
        {
            std::array<int, 5> values;
            values.fill(0);
            std::string_view::const_iterator x = column.begin();
            
            size_t val_index = 0;
            for (;val_index < values.size() && x != column.end(); ++val_index)
            {
                values[val_index] = decode_vlq(x);
            }
            
            switch (val_index - 1)
            {
            case kNameIdx_Idx:
                accum.nameIndex += values[kNameIdx_Idx];
                [[fallthrough]];
            case kSourceColumn_Idx:
                accum.srcColumn += values[kSourceColumn_Idx];
                [[fallthrough]];
            case kSourceLine_Idx:
                accum.srcLine += values[kSourceLine_Idx];
                [[fallthrough]];
            case kSourceId_Idx:
                accum.srcIndex += values[kSourceLine_Idx];
                [[fallthrough]];
            case kDstColumn_Idx:
                accum.dstColumn += values[kDstColumn_Idx];
                break;
            default:
                MY_DEBUG_FATAL("Bad source map entry ({})", column);
            }
            

            columns.emplace_back(accum);
            std::cout << std::format(" [{}]=[{},{},{},{},{}]\n", column, values[0], values[1], values[2], values[3], values[4]);
            /*
            
            
            while (x != column.end())
            {
                *i = decode_vlq(x);
                ++i;
            }*/
        }
        std::cout << "--------------------------------" << std::endl;
    }

    std::cout << "Parse completed\n";
}

}  // namespace my::test
