// #my_engine_source_file
#include "dap_utils.h"
#include "my/diag/logging.h"
#include "my/io/memory_stream.h"
#include "my/memory/buffer.h"
#include "my/memory/runtime_stack.h"
#include "my/network/http_parser.h"
#include "my/serialization/json.h"
#include "my/serialization/runtime_value_builder.h"

using namespace my::async;

namespace my::dap {

async::Task<RawPacket> readHttpPacket(io::IAsyncStream& stream)
{
    Buffer packet;
    HttpParser http;

    while (!http)
    {
        if (Buffer buffer = co_await stream.read())
        {
            packet.concat(std::move(buffer));
        }
        else
        {
            co_return MakeError("Unexpected end of steram");
        }

        http = HttpParser{asStringView(packet)};
    }

    if (!http.hasHeader("Content-Length"))
    {
        // TODO: handle invalid packet
        co_return MakeError("Invalid packet: no Content-Length");
    }

    co_return RawPacket{.buffer = std::move(packet), .http = http};
}

Result<Ptr<ReadonlyDictionary>> parseDapRequestToDictionary(const RawPacket& packet)
{
    const auto& [buffer, http] = packet;

    const char* const contentPtr = reinterpret_cast<const char*>(buffer.data()) + http.headersLength();
    const std::string_view content = std::string_view{contentPtr, http.contentLength()};
    mylog_debug("DAP: Incomig request, len:({}):\n{}", content.size(), content);

    Result<RuntimeValuePtr> parseResult = serialization::jsonParseString(content);
    CheckResult(parseResult);

    if (!(*parseResult)->is<ReadonlyDictionary>())
    {
        return MakeError("Invalid json document: expexted dictionary");
    }

    return parseResult;
}

Result<RequestMessage> parseDapRequest(const RawPacket& packet)
{
    Result<Ptr<ReadonlyDictionary>> dict = parseDapRequestToDictionary(packet);
    CheckResult(dict);

    RequestMessage request;
    CheckResult(runtimeValueApply(request, *dict));

    return Result{std::move(request)};
}

// async::Task<Ptr<ReadonlyDictionary>> readDapHttpRequest(io::IAsyncStream& stream)
// {
//     RawPacket packet = co_await readHttpPacket(stream);

//     co_return parseDapRequest(packet);
// }

Buffer makeDapHttpResponsePacket(std::string_view path, RuntimeValuePtr body)
{
   using namespace my::io;

    Buffer responseContent;

    {
        rtstack_scope;
        MemoryStreamPtr contentStream = createMemoryStream(AccessMode::Write, GetRtStackAllocatorPtr());

        constexpr bool PrettyPrint = true;  // TODO: make only for debug
        serialization::jsonWrite(*contentStream, body, serialization::JsonSettings{.pretty = PrettyPrint, .writeNulls = false}).ignore();
        responseContent = contentStream->as<WrittableMemoryStream&>().releaseBuffer();
    }

    std::string headers = std::format("POST {} HTTP/1.1\r\n", path);
    headers = headers +
              "Content-Type: application/json\r\n"
              "Content-Length: " +
              std::to_string(responseContent.size()) +
              "\r\n\r\n";

    Buffer responsePacket;
    responsePacket.resize(headers.size() + responseContent.size());
    memcpy(responsePacket.data(), headers.data(), headers.size());
    memcpy(responsePacket.data() + headers.size(), responseContent.data(), responseContent.size());

    return responsePacket;
}

// async::Task<> sendDapHttpResponse(io::IAsyncStream& stream, std::string_view path, RuntimeValuePtr body)
// {
//     Buffer
//     mylog_debug("DAP: outgoing response:\n{}", asStringView(responsePacket));

//     return stream.write(responsePacket.toReadOnly());
// }

}  // namespace my::dap
