// #my_engine_source_header


#pragma once

#include "my/utils/type_list/type_list.h"

namespace my::type_list
{

    template <template <typename, auto...> class Mapper, typename... T>
    constexpr auto transform_t(TypeList<T...>)
    {
        return TypeList<typename Mapper<T>::type...>{};
    }

    template <template <typename, auto...> class Mapper, typename... T>
    constexpr auto transform(TypeList<T...>)
    {
        return TypeList<Mapper<T>...>{};
    }

    template <typename TL, template <typename, auto...> class Mapper>
    using Transform = decltype(transform<Mapper>(TL{}));

    template <typename TL, template <typename, auto...> class Mapper>
    using TransformT = decltype(transform_t<Mapper>(TL{}));

}  // namespace my::type_list
