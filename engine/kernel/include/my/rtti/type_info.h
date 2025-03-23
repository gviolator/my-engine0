// #my_engine_source_header
#pragma once
#include <compare>
#include <concepts>
#include <string_view>
#include <type_traits>

#include "my/kernel/kernel_config.h"
#include "my/meta/class_info.h"
#include "my/utils/string_hash.h"
#include "my/utils/type_tag.h"

#if defined(_MSC_VER)
    #define MY_ABSTRACT_TYPE __declspec(novtable)
#else
    #define MY_ABSTRACT_TYPE
#endif

namespace my::rtti_detail
{
    struct TypeId
    {
        size_t typeId = 0;

        constexpr TypeId(size_t tid) :
            typeId{tid}
        {
        }

        constexpr TypeId() = default;
        constexpr TypeId(const TypeId&) = default;
        constexpr TypeId& operator=(const TypeId&) = default;
        constexpr explicit operator bool() const
        {
            return typeId > 0;
        }

        constexpr explicit operator size_t() const
        {
            return typeId;
        }

        auto operator<=>(const TypeId&) const noexcept = default;
    };

    template <typename T>
    struct DeclaredTypeId : std::false_type
    {
    };

    template <typename T>
    concept Concept_RttiTypeId = requires {
        { T::NauRtti_TypeId } -> std::same_as<const TypeId&>;
    };

    MY_KERNEL_EXPORT TypeId registerRuntimeTypeInternal(size_t typeHash, std::string_view typeName);

    template <std::size_t N>
    inline TypeId registerRuntimeType(const char (&typeName)[N])
    {
        return registerRuntimeTypeInternal(strings::constHash(typeName), std::string_view{typeName, N});
    }

}  // namespace my::rtti_detail

// clang-format on

#define MY_TYPEID(TypeName)                                                                \
public:                                                                                    \
    static_assert(std::is_trivial_v<::my::TypeTag<TypeName>>, "Check actual type exists"); \
    [[maybe_unused]]                                                                       \
    static inline const ::my::rtti_detail::TypeId NauRtti_TypeId = ::my::rtti_detail::registerRuntimeType(#TypeName);

#define MY_DECLARE_TYPEID(TypeName)                                                                                                     \
    namespace my::rtti_detail                                                                                                           \
    {                                                                                                                                   \
        template <>                                                                                                                     \
        struct DeclaredTypeId<TypeName> : std::true_type                                                                                \
        {                                                                                                                               \
            static inline const ::my::rtti_detail::TypeId NauRtti_TypeId = ::my::rtti_detail::registerRuntimeType(#TypeName); \
        };                                                                                                                              \
    }

// clang-format on

namespace my::rtti
{
    class TypeInfo;

    template <typename T>
    inline constexpr bool HasTypeInfo = rtti_detail::Concept_RttiTypeId<T> || rtti_detail::DeclaredTypeId<T>::value;

    template <typename T>
    concept WithTypeInfo = HasTypeInfo<T>;

    template <typename T>
    concept ClassWithTypeInfo = HasTypeInfo<T> && !std::is_abstract_v<T>;

    template <WithTypeInfo T>
    const TypeInfo& getTypeInfo();

    /**
     */
    class [[nodiscard]] MY_KERNEL_EXPORT TypeInfo
    {
    public:
        static TypeInfo fromId(size_t typeId);
        static TypeInfo fromName(const char* name);

        TypeInfo() = default;
        TypeInfo(const TypeInfo&) = default;
        TypeInfo& operator=(const TypeInfo&) = default;

        constexpr inline size_t getHashCode() const
        {
            return static_cast<size_t>(m_typeId);
        }

        constexpr std::string_view getTypeName() const
        {
            return m_typeName;
        }

        constexpr explicit operator bool() const noexcept
        {
            return static_cast<bool>(m_typeId);
        }

        auto operator<=>(const TypeInfo&) const noexcept = default;

    private:
        constexpr TypeInfo(const rtti_detail::TypeId typeId, std::string_view typeName) :
            m_typeId(typeId),
            m_typeName(typeName)
        {
        }

        rtti_detail::TypeId m_typeId;
        std::string_view m_typeName;
    };

    /*
     */
    template <WithTypeInfo T>
    const TypeInfo& getTypeInfo()
    {
        using namespace my::rtti_detail;

        static const TypeInfo typeInfo = []
        {
            if constexpr (Concept_RttiTypeId<T>)
            {
                return TypeInfo::fromId(static_cast<size_t>(T::NauRtti_TypeId));
            }
            else
            {
                static_assert(DeclaredTypeId<T>::value, "TypeId is not declared");
                return TypeInfo::fromId(static_cast<size_t>(DeclaredTypeId<T>::NauRtti_TypeId));
            }
        }();

        return (typeInfo);
    }

}  // namespace my::rtti

template <>
struct std::hash<::my::rtti::TypeInfo>
{
    [[nodiscard]]
    size_t operator()(const ::my::rtti::TypeInfo& val) const
    {
        return val.getHashCode();
    }
};
