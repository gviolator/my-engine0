// #my_engine_source_file

#pragma once
#include "my/utils/type_list/type_list.h"

namespace my::type_list_detail
{

    template <typename, typename...>
    struct Append;

    template <typename, typename...>
    struct AppendHead;

    template <typename... T, typename... El>
    struct Append<TypeList<T...>, El...>
    {
        using type = TypeList<T..., El...>;
    };

    template <typename... T, typename... El>
    struct AppendHead<TypeList<T...>, El...>
    {
        using type = TypeList<El..., T...>;
    };

}  // namespace my::type_list_detail

namespace my::type_list
{

    template <typename TL, typename T, typename... U>
    using Append = typename my::type_list_detail::Append<TL, T, U...>::type;

    template <typename TL, typename T, typename... U>
    using AppendHead = typename my::type_list_detail::AppendHead<TL, T, U...>::type;

}  // namespace my::type_list
