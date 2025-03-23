// #my_engine_source_header


#pragma once

#include <utility>

#include "my/utils/type_list/type_list.h"

namespace my::type_list_detail
{
    template <typename T, auto>
    struct GetT
    {
        using type = T;
    };

    template <typename, typename>
    struct FillHelper;

    template <typename T, size_t... I>
    struct FillHelper<T, std::index_sequence<I...>>
    {
        using type = TypeList<typename GetT<T, I>::type...>;
    };

    template <typename T>
    struct FillHelper<T, std::index_sequence<>>
    {
        using type = TypeList<>;
    };
}  // namespace my::type_list_detail

namespace my::type_list
{
    template <typename T, size_t Size>
    using Fill = typename ::my::type_list_detail::FillHelper<T, std::make_index_sequence<Size>>::type;
}  // namespace my::type_list
