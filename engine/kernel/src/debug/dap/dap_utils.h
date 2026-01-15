// #my_engine_source_file
#pragma once
#include "my/debug/dap/dap.h"
#include "my/diag/assert.h"
#include "my/io/async_stream.h"
#include "my/memory/runtime_stack.h"
#include "my/network/http_parser.h"
#include "my/serialization/runtime_value.h"
#include "my/serialization/runtime_value_builder.h"
#include "my/utils/result.h"

#include <concepts>

namespace my::dap {

struct RawPacket
{
    Buffer buffer;
    HttpParser http;
};

async::Task<RawPacket> readHttpPacket(io::IAsyncStream& stream);

Result<Ptr<ReadonlyDictionary>> parseDapRequestToDictionary(const RawPacket& packet);

Result<RequestMessage> parseDapRequest(const RawPacket& packet);

// async::Task<Ptr<ReadonlyDictionary>> readDapHttpRequest(io::IAsyncStream& stream);

// async::Task<> sendDapHttpResponse(io::IAsyncStream& stream, std::string_view path, RuntimeValuePtr body);

// async::Task<> sendDapHttpResponse(io::IAsyncStream& stream, std::string_view path, RuntimeValuePtr body);
Buffer makeDapHttpResponsePacket(std::string_view path, RuntimeValuePtr body);

//template < T>
async::Task<> sendResponse(io::IAsyncStream& stream, const std::derived_from<dap::ResponseMessage> auto&  responseMessage)
{
    Buffer packet;
    {
        rtstack_scope;
        auto value = makeValueRef(responseMessage, getRtStackAllocatorPtr());
        packet = makeDapHttpResponsePacket("/dap", std::move(value));
    }

    mylog_debug("DAP: outgoing response:\n{}", asStringView(packet));
    co_await stream.write(packet.toReadOnly());

    mylog_debug("Response on fly");
}

async::Task<> sendEvent(io::IAsyncStream& stream, const std::derived_from<dap::EventMessage> auto& message)
{
    Buffer packet;
    {
        rtstack_scope;
        auto value = makeValueRef(message, getRtStackAllocatorPtr());
        packet = makeDapHttpResponsePacket("/dap", std::move(value));
    }

    mylog_debug("DAP: outgoing event:\n{}", asStringView(packet));
    co_await stream.write(packet.toReadOnly());

    mylog_debug("event on fly");
}


}  // namespace my::dap
