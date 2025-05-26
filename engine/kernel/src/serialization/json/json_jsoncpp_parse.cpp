// #my_engine_source_file

#include "json_to_runtime_value.h"
#include "my/io/stream.h"
#include "my/memory/buffer.h"

namespace my::json_detail
{
    inline std::unique_ptr<Json::CharReader> makeJsonCharReader()
    {
        Json::CharReaderBuilder builder;
        return std::unique_ptr<Json::CharReader>{builder.newCharReader()};
    }
}  // namespace my::json_detail

namespace my::serialization
{
    Json::CharReader& jsonGetParser()
    {
        static thread_local auto parser = json_detail::makeJsonCharReader();
        return (*parser);
    }

    Result<RuntimeValuePtr> jsonParse(io::IStream& reader, IMemAllocator* allocator)
    {
        MY_DEBUG_ASSERT(reader.canRead());

        constexpr size_t BlockSize = 256;

        Buffer buffer;
        size_t totalRead = 0;

        do
        {
            std::byte* const ptr = reinterpret_cast<std::byte*>(buffer.append(BlockSize));
            auto readResult = reader.read(ptr, BlockSize);
            CheckResult(readResult);

            const size_t actualRead = *readResult;
            totalRead += actualRead;

            if (actualRead < BlockSize)
            {
                buffer.resize(totalRead);
                break;
            }

        } while (true);

        std::string_view str{reinterpret_cast<const char*>(buffer.data()), buffer.size()};
        return jsonParseString(str, allocator);
    }

    Result<Json::Value> jsonParseToValue(std::string_view str)
    {
        if (str.empty())
        {
            return MakeError("Empty string");
        }

        std::string message;
        Json::Value root;

        if (!jsonGetParser().parse(str.data(), str.data() + str.size(), &root, &message))
        {
            return MakeErrorT(SerializationError)(std::string{message.data(), message.size()});
        }

        return root;
    }

    Result<RuntimeValuePtr> jsonParseString(std::string_view str, IMemAllocator*)
    {
        auto root = jsonParseToValue(str);
        CheckResult(root);

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
