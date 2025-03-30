// #my_engine_source_file
#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>

#include "my/kernel/kernel_config.h"

namespace my::threading
{
    /**
     *
     */
    class MY_KERNEL_EXPORT Barrier
    {
    public:
        Barrier(size_t total);
        Barrier(const Barrier&) = delete;
        bool enter(std::optional<std::chrono::milliseconds> timeout = std::nullopt);

    private:
        const size_t m_total;
        std::atomic<size_t> m_counter{0};
        std::mutex m_mutex;
        std::condition_variable m_signal;
    };

}  // namespace my::threading
