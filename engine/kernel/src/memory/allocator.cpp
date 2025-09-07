// #my_engine_source_file
#include "crt_allocator.h"


namespace my {

IAllocator& getDefaultAllocator()
{
    static Ptr<CrtAllocator> allocator = rtti::createInstanceSingleton<CrtAllocator>();
    return *allocator;
}

IAllocator& getDefaultAlignedAllocator()
{
    static Ptr<AlignedCrtAllocator> allocator = rtti::createInstanceSingleton<AlignedCrtAllocator>();
    return *allocator;
}

}  // namespace my
