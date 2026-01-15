// #my_engine_source_file
#include "my/diag/assert.h"

#include <concepts>
#include <cstdlib>
#include <limits>
#include <type_traits>

namespace my {
template <std::integral T, std::integral U>
constexpr T numeric_cast(const U source)
{
    if constexpr (std::is_same_v<T, U>)
    {
        return source;
    }
    else if constexpr (sizeof(U) > sizeof(T))
    {
        constexpr U MaxT = static_cast<U>(std::numeric_limits<T>::max());
        MY_DEBUG_ASSERT(std::abs(source) <= MaxT, "Numeric overflow ({}) too big for target type", source);

        return static_cast<U>(source);
    }
}
}  // namespace my