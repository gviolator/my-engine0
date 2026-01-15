#pragma once
#include "lua_toolkit/lua_header.h"
#include "my/io/stream.h"
#include "my/memory/allocator.h"
#include "my/memory/mem_base.h"

namespace my::lua {

struct ChunkLoader
{
    io::IStream& stream;
    IAllocator& allocator;
    std::byte* buffer = nullptr;
    const size_t bufferSize;

    ChunkLoader(io::IStream& inStream, IAllocator& inAllocator, size_t size) :
        stream(inStream),
        allocator(inAllocator),
        bufferSize(alignedSize(size, 64))
    {
        buffer = reinterpret_cast<std::byte*>(allocator.alloc(bufferSize));
        MY_FATAL(buffer);
    }

    ~ChunkLoader()
    {
        allocator.free(buffer);
    }

    static const char* read([[maybe_unused]] lua_State* lua, void* data, size_t* size) noexcept
    {
        auto& loader = *reinterpret_cast<ChunkLoader*>(data);

        Result<size_t> readResult = loader.stream.read(loader.buffer, loader.bufferSize);
        if (!readResult)
        {
            MY_FAILURE("Fail to read input stream: ({})", readResult.getError()->getMessage());
            *size = 0;
            return nullptr;
        }

        *size = *readResult;
        return *size > 0 ? reinterpret_cast<const char*>(loader.buffer) : nullptr;
    }
};
}  // namespace my::lua
