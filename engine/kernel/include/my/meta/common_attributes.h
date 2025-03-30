// #my_engine_source_file

#pragma once
#include <EASTL/string.h>

#include <type_traits>

#include "my/meta/attribute.h"

namespace my::meta
{
    MY_DEFINE_ATTRIBUTE(ClassNameAttribute, "nau.class.name", AttributeOptionsNone)

    template <typename T>
    inline constexpr bool ClassHasName = AttributeDefined<T, ClassNameAttribute>;

    /**
     */
    template <typename T>
    std::string getClassName()
    {
        if constexpr (AttributeDefined<T, ClassNameAttribute>)
        {
            static_assert(std::is_constructible_v<std::string, AttributeValueType<T, ClassNameAttribute>>);
            return meta::getAttributeValue<T, ClassNameAttribute>();
        }
        else
        {
            return std::string{};
        }
    }

}  // namespace my::meta

#define CLASS_NAME_ATTRIBUTE(ClassName) CLASS_ATTRIBUTE(my::meta::ClassNameAttribute, std::string{ClassName})
