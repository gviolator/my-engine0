// #my_engine_source_file

#pragma once
#include "my/diag/assert.h"

namespace my
{
    constexpr inline bool is_power_of2(size_t value)
    {
        return (value & (value - 1)) == 0;
    }

    constexpr inline size_t aligned_size(size_t size, size_t alignment)
    {
        MY_DEBUG_ASSERT(is_power_of2(alignment), "alignment expected to be a power of two. Actual value: ({})", alignment);
        return (size + alignment - 1) & ~(alignment - 1);
    }

    template <size_t Factor>
    struct BytesValue
    {
        static inline constexpr size_t BytesFactor = Factor;

        const size_t count;

        constexpr BytesValue(size_t value = 0) :
            count(value)
        {
        }

        template <size_t U>
        constexpr BytesValue(const BytesValue<U>& other) :
            count((U / Factor) * other.count)
        {
        }

        constexpr operator size_t() const
        {
            return this->bytesCount();
        }

        constexpr size_t bytesCount() const
        {
            return count * Factor;
        }

        template <size_t U>
        constexpr BytesValue& operator=(const BytesValue<U>& other)
        {
            this->count = (U / Factor) * other.count();
            return *this;
        }
    };

    template <size_t U, size_t V>
    inline constexpr auto operator+(const BytesValue<U>& u, const BytesValue<V>& v)
    {
        if constexpr (U < V)
        {
            return BytesValue<U>(BytesValue<1>{u.bytesCount() + v.bytesCount()});
        }
        else
        {
            return BytesValue<V>(BytesValue<1>{u.bytesCount() + v.bytesCount()});
        }
    }

    using Byte = BytesValue<1>;
    using Kilobyte = BytesValue<1024>;
    using Megabyte = BytesValue<1024 * 1024>;

    namespace my_literals
    {

        [[nodiscard]]
        inline constexpr Byte
        operator""_b(unsigned long long count)
        {
            return Byte{static_cast<size_t>(count)};
        }

        [[nodiscard]]
        inline constexpr Kilobyte
        operator""_Kb(unsigned long long count)
        {
            return Kilobyte{static_cast<size_t>(count)};
        }

        [[nodiscard]]
        inline constexpr Megabyte
        operator""_Mb(unsigned long long count)
        {
            return Megabyte{static_cast<size_t>(count)};
        }
    }  // namespace my_literals

    namespace mem
    {
        constexpr inline size_t PageSize = Kilobyte(4);
        constexpr inline size_t AllocationGranularity = Kilobyte(64);
    }  // namespace mem
}  // namespace my
