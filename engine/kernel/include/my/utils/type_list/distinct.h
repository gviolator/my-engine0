// #my_engine_source_header

#pragma once
#include "my/utils/type_list/type_list.h"

namespace my::type_list_detail
{
    template <typename... R>
    constexpr TypeList<R...> distinct(TypeList<>, TypeList<R...> r)
    {
        return r;
    }

    template <typename H, typename... T, typename... R>
    constexpr auto distinct([[maybe_unused]] TypeList<H, T...> t, TypeList<R...> r)
    {
        if constexpr (type_list::findIndex<H>(r) < 0)
        {
            return distinct(TypeList<T...>{}, TypeList<R..., H>{});
        }
        else
        {
            return distinct(TypeList<T...>{}, r);
        }
    }

}  // namespace my::type_list_detail

namespace my::type_list
{

    template <typename... T>
    constexpr auto distinct(TypeList<T...> t)
    {
        return my::type_list_detail::distinct(t, TypeList<>{});
    }

    template <typename TL>
    using Distinct = decltype(type_list::distinct(TL{}));

}  // namespace my::type_list
