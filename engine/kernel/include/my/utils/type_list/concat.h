// #my_engine_source_header


#pragma once
#include "my/utils/type_list/type_list.h"

namespace my::type_list_detail
{

    template <typename... T1, typename... T2>
    constexpr auto concat2List(TypeList<T1...>, TypeList<T2...>)
    {
        return TypeList<T1..., T2...>{};
    }

}  // namespace my::type_list_detail

namespace my::type_list
{
    consteval TypeList<> concat()
    {
        return TypeList<>{};
    }

    template <typename H, typename... T>
    consteval auto concat(H h, T... t)
    {
        return my::type_list_detail::concat2List(h, type_list::concat(t...));
    }

    template <typename... TL>
    using Concat = decltype(type_list::concat(TL{}...));

}  // namespace my::type_list
