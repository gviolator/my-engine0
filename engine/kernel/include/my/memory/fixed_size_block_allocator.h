// #my_engine_source_file
#pragma once
#include "my/kernel/kernel_config.h"
#include "my/memory/host_memory.h"
#include "my/memory/mem_allocator.h"

namespace my
{
    MY_KERNEL_EXPORT MemAllocatorPtr createFixedSizeBlockAllocator(HostMemoryPtr hostMemory, Byte blockSize, bool threadSafe);
} // namespace my
