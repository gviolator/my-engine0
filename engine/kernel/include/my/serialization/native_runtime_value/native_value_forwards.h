// #my_engine_source_file
#pragma once

#include <array>
#include <concepts>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_set>

#include "my/memory/mem_allocator.h"
#include "my/meta/class_info.h"
#include "my/serialization/runtime_value.h"
#include "my/utils/to_string.h"
#include "my/utils/tuple_utility.h"
#include "my/utils/type_list/fill.h"
#include "my/utils/type_utility.h"

namespace my::ser_detail
{
    template <typename T>
    struct KnownSetCollection : std::false_type
    {
    };

    template <typename T, typename... Traits>
    struct KnownSetCollection<std::set<T, Traits...>> : std::true_type
    {
    };

    template <typename T, typename... Traits>
    struct KnownSetCollection<std::unordered_set<T, Traits...>> : std::true_type
    {
    };
}  // namespace my::ser_detail

/**

*/

namespace my
{
    /**
     */
    template <typename T>
    concept LikeStdCollection = requires(const T& collection) {
        typename T::value_type;
        typename T::size_type;
        { collection.size() } -> std::same_as<typename T::size_type>;
        { collection.begin() } -> std::template same_as<typename T::const_iterator>;
        { collection.front() } -> std::template same_as<typename T::const_reference>;
        { collection.back() } -> std::template same_as<typename T::const_reference>;
    } && requires(T& collection) {
        collection.clear();
        { collection.begin() } -> std::template same_as<typename T::iterator>;
        { collection.front() } -> std::template same_as<typename T::reference>;
        { collection.back() } -> std::template same_as<typename T::reference>;
        collection.emplace_back();  // } -> std::template same_as<typename T::reference>;
    };

    /**
     */
    template <typename T>
    concept LikeStdVector = requires(const T& collection) {
        typename T::value_type;
        typename T::size_type;
        { collection.operator[](0) } -> std::template same_as<typename T::const_reference>;
    } && requires(T& collection) {
        { collection.operator[](0) } -> std::template same_as<typename T::reference>;
    } && LikeStdCollection<T>;

    /**
     */
    template <typename T>
    concept LikeStdList = requires(T& collection) {
        collection.emplace_front();
    } && LikeStdCollection<T>;

    /**
     */
    template <typename T>
    concept LikeStdMap = requires(const T& dict) {
        typename T::key_type;
        typename T::value_type;

        { dict.size() } -> std::same_as<typename T::size_type>;
        { dict.begin() } -> std::same_as<typename T::const_iterator>;
        { dict.end() } -> std::same_as<typename T::const_iterator>;
        { dict.find(constLValueRef<typename T::key_type>()) } -> std::same_as<typename T::const_iterator>;
    } && requires(T& dict) {
        dict.clear();
        { dict.begin() } -> std::same_as<typename T::iterator>;
        { dict.end() } -> std::same_as<typename T::iterator>;
        { dict.find(constLValueRef<typename T::key_type>()) } -> std::same_as<typename T::iterator>;
        dict.try_emplace(constLValueRef<typename T::key_type>());
    };

    /**
        Using restricted set types to minimize collisions with other declarations.
    */
    template <typename Collection>
    concept LikeSet = ser_detail::KnownSetCollection<Collection>::value;

    /**
     */
    template <typename T>
    concept LikeStdOptional = requires(const T& opt) {
        { opt.has_value() } -> std::template same_as<bool>;
    } && requires(T& opt) {
        opt.reset();
        opt.emplace();
        { opt.value() } -> std::template same_as<typename T::value_type&>;
    };

    /**
     */
    // template <typename T>
    // concept StringParsable = requires(const T& value) {
    //     {
    //         toString(value)
    //     } -> std::same_as<std::string>;
    // } && requires(T& value) {
    //     {
    //         parse(std::string_view{}, value)
    //     } -> std::same_as<Result<>>;
    // };

    template <typename T>
    concept WithOwnRuntimeValue = requires {
        T::HasOwnRuntimeValue;
    } && T::HasOwnRuntimeValue;

    template <typename T>
    concept AutoStringRepresentable = StringParsable<T> && !WithOwnRuntimeValue<T>;

    /**
     */
    template <typename>
    struct TupleValueOperations1 : std::false_type
    {
    };

    template <typename>
    struct UniformTupleValueOperations : std::false_type
    {
    };

    template <typename... T>
    struct TupleValueOperations1<std::tuple<T...>> : std::true_type
    {
        static inline constexpr size_t TupleSize = sizeof...(T);

        using Type = std::tuple<T...>;
        using Elements = TypeList<T...>;

        template <size_t Index>
        static decltype(auto) element(Type& tup)
        {
            static_assert(Index < TupleSize, "Invalid tuple element index");
            return std::get<Index>(tup);
        }

        template <size_t Index>
        static decltype(auto) element(const Type& tup)
        {
            static_assert(Index < TupleSize, "Invalid tuple element index");
            return std::get<Index>(tup);
        }
    };

    template <>
    struct TupleValueOperations1<std::tuple<>> : std::true_type
    {
        static inline constexpr size_t TupleSize = 0;

        using Type = std::tuple<>;
        using Elements = TypeList<>;

        template <size_t Index>
        static std::nullopt_t element(const Type&)
        {
            static_assert(Index == 0, "Invalid tuple element index");
            return std::nullopt;
        }
    };

    template <typename First, typename Second>
    struct TupleValueOperations1<std::pair<First, Second>>
    {
        static constexpr size_t TupleSize = 2;

        using Type = std::pair<First, Second>;
        using Elements = TypeList<First, Second>;

        template <size_t Index>
        static decltype(auto) element(Type& tup)
        {
            static_assert(Index < TupleSize, "Invalid pair index");
            return std::get<Index>(tup);
        }

        template <size_t Index>
        static decltype(auto) element(const Type& tup)
        {
            static_assert(Index < TupleSize, "Invalid pair index");
            return std::get<Index>(tup);
        }
    };

    template <typename T, size_t Size>
    struct UniformTupleValueOperations<std::array<T, Size>> : std::true_type
    {
        static constexpr size_t TupleSize = Size;
        using Type = std::array<T, Size>;
        using Element = T;

        static_assert(Size > 0, "std::array<T,0> does not supported");

        static decltype(auto) element(Type& tup, size_t index)
        {
            MY_DEBUG_ASSERT(index < Size, "[{}], size():{}", index, TupleSize);
            return tup[index];
        }

        static decltype(auto) element(const Type& tup, size_t index)
        {
            MY_DEBUG_ASSERT(index < Size, "[{}], size():{}", index, TupleSize);
            return tup[index];
        }
    };

    template <typename T>
    concept NauClassWithFields = meta::ClassHasFields<T>;

    template <typename T>
    concept LikeTuple = TupleValueOperations1<T>::value;

    template <typename T>
    concept LikeUniformTuple = UniformTupleValueOperations<T>::value;

    Ptr<RuntimeValueRef> makeValueRef(const RuntimeValuePtr& value, IMemAllocator* = nullptr);

    Ptr<RuntimeValueRef> makeValueRef(RuntimeValuePtr& value, IMemAllocator* = nullptr);

    // Integral
    template <std::integral T>
    Ptr<RuntimeIntegerValue> makeValueRef(T&, IMemAllocator* = nullptr);

    template <std::integral T>
    Ptr<RuntimeIntegerValue> makeValueRef(const T&, IMemAllocator* = nullptr);

    template <std::integral T>
    Ptr<RuntimeIntegerValue> makeValueCopy(T, IMemAllocator* = nullptr);

    // Boolean
    Ptr<RuntimeBooleanValue> makeValueRef(bool&, IMemAllocator* = nullptr);

    Ptr<RuntimeBooleanValue> makeValueRef(const bool&, IMemAllocator* = nullptr);

    Ptr<RuntimeBooleanValue> makeValueCopy(bool, IMemAllocator* = nullptr);

    // Floating point
    template <std::floating_point T>
    Ptr<RuntimeFloatValue> makeValueRef(T&, IMemAllocator* = nullptr);

    template <std::floating_point T>
    Ptr<RuntimeFloatValue> makeValueRef(const T&, IMemAllocator* = nullptr);

    template <std::floating_point T>
    Ptr<RuntimeFloatValue> makeValueCopy(T, IMemAllocator* = nullptr);

    // String
    template <typename... Traits>
    Ptr<RuntimeStringValue> makeValueRef(std::basic_string<char, Traits...>&, IMemAllocator* = nullptr);

    template <typename... Traits>
    Ptr<RuntimeStringValue> makeValueRef(const std::basic_string<char, Traits...>&, IMemAllocator* = nullptr);

    Ptr<RuntimeStringValue> makeValueCopy(std::string_view, IMemAllocator* = nullptr);

    template <AutoStringRepresentable T>
    Ptr<RuntimeStringValue> makeValueRef(T&, IMemAllocator* = nullptr);

    template <AutoStringRepresentable T>
    Ptr<RuntimeStringValue> makeValueRef(const T&, IMemAllocator* = nullptr);

    template <AutoStringRepresentable T>
    Ptr<RuntimeStringValue> makeValueCopy(const T&, IMemAllocator* = nullptr);

    template <AutoStringRepresentable T>
    Ptr<RuntimeStringValue> makeValueCopy(T&&, IMemAllocator* = nullptr);

    // Optional
    template <LikeStdOptional T>
    Ptr<RuntimeOptionalValue> makeValueRef(T&, IMemAllocator* = nullptr);

    template <LikeStdOptional T>
    Ptr<RuntimeOptionalValue> makeValueRef(const T&, IMemAllocator* = nullptr);

    template <LikeStdOptional T>
    Ptr<RuntimeOptionalValue> makeValueCopy(const T&, IMemAllocator* = nullptr);

    template <LikeStdOptional T>
    Ptr<RuntimeOptionalValue> makeValueCopy(T&&, IMemAllocator* = nullptr);

    // Tuple
    template <LikeTuple T>
    Ptr<RuntimeReadonlyCollection> makeValueRef(T&, IMemAllocator* = nullptr);

    template <LikeTuple T>
    Ptr<RuntimeReadonlyCollection> makeValueRef(const T&, IMemAllocator* = nullptr);

    template <LikeTuple T>
    Ptr<RuntimeReadonlyCollection> makeValueCopy(const T&, IMemAllocator* = nullptr);

    template <LikeTuple T>
    Ptr<RuntimeReadonlyCollection> makeValueCopy(T&&, IMemAllocator* = nullptr);

    template <LikeUniformTuple T>
    Ptr<RuntimeReadonlyCollection> makeValueRef(T&, IMemAllocator* = nullptr);

    template <LikeUniformTuple T>
    Ptr<RuntimeReadonlyCollection> makeValueRef(const T&, IMemAllocator* = nullptr);

    template <LikeUniformTuple T>
    Ptr<RuntimeReadonlyCollection> makeValueCopy(const T&, IMemAllocator* = nullptr);

    template <LikeUniformTuple T>
    Ptr<RuntimeReadonlyCollection> makeValueCopy(T&&, IMemAllocator* = nullptr);

    // Collection
    template <LikeStdVector T>
    Ptr<RuntimeCollection> makeValueRef(T&, IMemAllocator* = nullptr);

    template <LikeStdVector T>
    Ptr<RuntimeCollection> makeValueRef(const T&, IMemAllocator* = nullptr);

    template <LikeStdVector T>
    Ptr<RuntimeCollection> makeValueCopy(const T&, IMemAllocator* = nullptr);

    template <LikeStdVector T>
    Ptr<RuntimeCollection> makeValueCopy(T&&, IMemAllocator* = nullptr);

    template <LikeStdList T>
    Ptr<RuntimeCollection> makeValueRef(T&, IMemAllocator* = nullptr);

    template <LikeStdList T>
    Ptr<RuntimeCollection> makeValueRef(const T&, IMemAllocator* = nullptr);

    template <LikeStdList T>
    Ptr<RuntimeCollection> makeValueCopy(const T&, IMemAllocator* = nullptr);

    template <LikeStdList T>
    Ptr<RuntimeCollection> makeValueCopy(T&&, IMemAllocator* = nullptr);

    template <LikeSet T>
    Ptr<RuntimeCollection> makeValueRef(T&, IMemAllocator* = nullptr);

    template <LikeSet T>
    Ptr<RuntimeCollection> makeValueRef(const T&, IMemAllocator* = nullptr);

    template <LikeSet T>
    Ptr<RuntimeCollection> makeValueCopy(const T&, IMemAllocator* = nullptr);

    template <LikeSet T>
    Ptr<RuntimeCollection> makeValueCopy(T&&, IMemAllocator* = nullptr);

    // Dictionary
    template <LikeStdMap T>
    Ptr<RuntimeDictionary> makeValueRef(T& dict, IMemAllocator* = nullptr);

    template <LikeStdMap T>
    Ptr<RuntimeDictionary> makeValueRef(const T& dict, IMemAllocator* = nullptr);

    template <LikeStdMap T>
    Ptr<RuntimeDictionary> makeValueCopy(const T& dict, IMemAllocator* = nullptr);

    template <LikeStdMap T>
    Ptr<RuntimeDictionary> makeValueCopy(T&& dict, IMemAllocator* = nullptr);

    // Object
    template <NauClassWithFields T>
    Ptr<RuntimeObject> makeValueRef(T& obj, IMemAllocator* = nullptr);

    template <NauClassWithFields T>
    Ptr<RuntimeObject> makeValueRef(const T& obj, IMemAllocator* = nullptr);

    template <NauClassWithFields T>
    Ptr<RuntimeObject> makeValueCopy(const T& obj, IMemAllocator* = nullptr);

    template <NauClassWithFields T>
    Ptr<RuntimeObject> makeValueCopy(T&& obj, IMemAllocator* = nullptr);

}  // namespace my

namespace my::ser_detail
{
    template <typename T>
    decltype(makeValueRef(lValueRef<T>()), std::true_type{}) hasMakeValueRefChecker(int);

    template <typename>
    std::false_type hasMakeValueRefChecker(...);

    template <typename T>
    decltype(makeValueCopy(constLValueRef<T>()), std::true_type{}) hasMakeValueCopyChecker(int);

    template <typename>
    std::false_type hasMakeValueCopyChecker(...);

    template <typename T>
    constexpr bool HasRtValueRepresentationHelper = decltype(hasMakeValueRefChecker<T>(int{0}))::value;

    template <typename T>
    constexpr bool HasRtValueCopyHelper = std::is_copy_constructible_v<T> && decltype(hasMakeValueCopyChecker<T>(int{0}))::value;

    template <typename... T>
    inline consteval bool allHasRuntimeValueRepresentation(TypeList<T...>)
    {
        return (HasRtValueRepresentationHelper<T> && ...);
    }

    template <typename T>
    consteval bool hasRtValueRepresentation()
    {
        return HasRtValueRepresentationHelper<T>;
    }

    template <LikeStdOptional T>
    consteval bool hasRtValueRepresentation()
    {
        using ValueType = typename T::value_type;
        return HasRtValueRepresentationHelper<ValueType>;
    }

    template <LikeTuple T>
    consteval bool hasRtValueRepresentation()
    {
        using Elements = typename TupleValueOperations1<T>::Elements;
        return allHasRuntimeValueRepresentation(Elements{});
    }

    template <LikeUniformTuple T>
    consteval bool hasRtValueRepresentation()
    {
        using Element = typename UniformTupleValueOperations<T>::Element;
        return HasRtValueRepresentationHelper<Element>;
    }

    template <LikeStdCollection T>
    consteval bool hasRtValueRepresentation()
    {
        return HasRtValueRepresentationHelper<typename T::value_type>;
    }

    template <LikeSet T>
    consteval bool hasRtValueRepresentation()
    {
        return HasRtValueRepresentationHelper<typename T::value_type>;
    }

    template <typename T>
    consteval bool hasMakeValueCopy()
    {
        return HasRtValueCopyHelper<T>;
    }

}  // namespace my::ser_detail

namespace my
{

    template <typename T>
    inline constexpr bool HasRuntimeValueRepresentation = ser_detail::hasRtValueRepresentation<T>();

    template <typename T>
    inline constexpr bool HasMakeValueCopy = ser_detail::hasMakeValueCopy<T>();

    template <typename T>
    concept RuntimeValueRepresentable = ser_detail::hasRtValueRepresentation<T>();

}  // namespace my
