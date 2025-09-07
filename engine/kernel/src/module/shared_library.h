// #my_engine_source_file
#pragma once

#include <uv.h>

#include <filesystem>
#include <optional>

#include "my/utils/result.h"


namespace my::os
{
    class SharedLibrary
    {
    public:
        static Result<SharedLibrary> open(const std::filesystem::path& dlPath);

        SharedLibrary(const SharedLibrary&) = delete;
        SharedLibrary(SharedLibrary&&);

        ~SharedLibrary();

        SharedLibrary& operator=(const SharedLibrary&) = delete;
        SharedLibrary& operator=(SharedLibrary&&);

        explicit operator bool() const;

        void close();

    private:
        SharedLibrary(const uv_lib_t& handle);

        std::optional<uv_lib_t> m_handle;
    };
}  // namespace my::os