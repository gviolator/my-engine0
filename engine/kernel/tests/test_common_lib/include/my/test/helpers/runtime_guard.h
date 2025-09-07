// #my_engine_source_file
#pragma once
#include <memory>

#include "my/runtime/internal/kernel_runtime.h"


namespace my::test {
class RuntimeGuard
{
public:
    using Ptr = std::unique_ptr<RuntimeGuard>;

    static RuntimeGuard::Ptr create();

    virtual ~RuntimeGuard() = default;

    virtual KernelRuntime& getKRuntime() = 0;

    virtual void reset() = 0;
};

}  // namespace my::test
