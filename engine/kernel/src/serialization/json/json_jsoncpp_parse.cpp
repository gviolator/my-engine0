#if 0
// #my_engine_source_file

#include "./json_to_runtime_value.h"
#include "nau/io/stream.h"
#include "nau/memory/bytes_buffer.h"


namespace my::json_detail
{
    inline eastl::unique_ptr<Json::CharReader> makeJsonCharReader()
    {
        Json::CharReaderBuilder builder;
        return eastl::unique_ptr<Json::CharReader>{builder.newCharReader()};
    }
}  // namespace my::json_detail

namespace my::serialization
{
    Json::CharReader& jsonGetParser()
    {
        static thread_local auto parser = json_detail::makeJsonCharReader();
        return (*parser);
    }

    Result<RuntimeValue::Ptr> jsonParse(io::IStreamReader& reader, IMemAllocator::Ptr allocator)
    {
        constexpr size_t BlockSize = 256;

        BytesBuffer buffer;
        size_t totalRead = 0;

        do
        {
            auto readResult = reader.read(buffer.append(BlockSize), BlockSize);
            NauCheckResult(readResult);

            const size_t actualRead = *readResult;
            totalRead += actualRead;

            if (actualRead < BlockSize)
            {
                buffer.resize(totalRead);
                break;
            }

        } while (true);

        eastl::u8string_view str{reinterpret_cast<const char8_t*>(buffer.data()), buffer.size()};
        return jsonParseString(str, std::move(allocator));
    }

    Result<Json::Value> jsonParseToValue(eastl::string_view str)
    {
        if (str.empty())
        {
            return NauMakeError("Empty string");
        }

        std::string message;
        Json::Value root;

        if (!jsonGetParser().parse(str.data(), str.data() + str.size(), &root, &message))
        {
            return NauMakeErrorT(SerializationError)(eastl::string{message.data(), message.size()});
        }

        return root;
    }

    Result<RuntimeValue::Ptr> jsonParseString(eastl::string_view str, IMemAllocator::Ptr)
    {
        auto root = jsonParseToValue(str);
        NauCheckResult(root);

        if (root->isObject())
        {
            return json_detail::createJsonDictionary(*std::move(root));
        }
        else if (root->isArray())
        {
            return json_detail::createJsonCollection(*std::move(root));
        }

        return json_detail::getValueFromJson(nullptr, *root);
    }

}  // namespace my::serialization

#endif