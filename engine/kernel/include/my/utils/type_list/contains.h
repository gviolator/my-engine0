// #my_engine_source_file


#pragma once
#include "my/utils/type_list/type_list.h"

namespace my::type_list
{
    template <typename U, typename... T>
    consteval bool contains(TypeList<T...>)
    {
        return (std::is_same_v<T, U> || ...);
    }

    template <typename... U, typename... T>
    consteval bool containsAll(TypeList<T...> l)
    {
        return (type_list::contains<U>(l) && ...);
    }

    template <typename TL, typename T>
    inline constexpr bool Contains = type_list::contains<T>(TL{});

}  // namespace my::type_list
