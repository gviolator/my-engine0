// #my_engine_source_header
#pragma once
#include <memory>

namespace my::test
{
    class RuntimeGuard
    {
    public:
        using Ptr = std::unique_ptr<RuntimeGuard>;

        static RuntimeGuard::Ptr create();

        virtual ~RuntimeGuard() = default;

        virtual void reset() = 0;
    };

}  // namespace nau::test
