// #my_engine_source_file
#pragma once

#include <array>
#include <span>
#include <string_view>
#include <type_traits>

#include "my/kernel/kernel_config.h"
#include "my/rtti/type_info.h"
#include "my/utils/result.h"
#include "my/utils/scope_guard.h"

namespace my
{
    template <typename T>
    concept EnumValueType = std::is_enum_v<T>;

    /**
        Runtime enum information (without knowing actual C++ enum type).
     */
    struct MY_ABSTRACT_TYPE IEnumRuntimeInfo
    {
        virtual ~IEnumRuntimeInfo() = default;

        virtual std::string_view getName() const = 0;

        virtual size_t getCount() const = 0;

        virtual std::span<const int> getIntValues() const = 0;

        virtual std::span<const std::string_view> getStringValues() const = 0;
    };

}  // namespace my

namespace my::kernel_detail
{
    struct EnumRuntimeInfoImpl : public IEnumRuntimeInfo
    {
        const char* typeName;
        const size_t itemCount;
        std::string_view const* const strValues = nullptr;
        int const* const intValues;

        EnumRuntimeInfoImpl(const char* inTypeName, size_t inItemCount, std::string_view* inStrValues, int* inIntValues) :
            typeName(inTypeName),
            itemCount(inItemCount),
            strValues(inStrValues),
            intValues(inIntValues)
        {
        }

        std::string_view getName() const override
        {
            return typeName;
        }

        size_t getCount() const override
        {
            return itemCount;
        }

        std::span<const int> getIntValues() const override
        {
            return {intValues, itemCount};
        }

        std::span<const std::string_view> getStringValues() const override
        {
            return {strValues, itemCount};
        }
    };

    /**
        Used to "simulate" enum items values assignment.
        The problem, that MY_DEFINE_ENUM accepts enum value items, but to refer to actual values code must
        specify enum's type names also.
        I.e
        MY_DEFINE_ENUM_(MyEnum, Value0, Value1)
        __VA_ARGS__ will have Value0, Value1, so we need to map they to MyEnum::Value0, MyEnum::Value1, that actually near impossible (for generic cases)

        So the 'trick' used inside MY_DEFINE_ENUM:
        - first, enum values passed as MY_DEFINE_ENUM macro parameters treat as variables EnumValueCounter<> (i.e. EnumValueCounter<MyEnum> Value0, Value1)
        - during variables declarations EnumValueCounter will assign values same way as C++ do (Value0.value = 0, Value1.value = 1)
        - next variables maps to array of actual enum values (by EnumTraitsHelper::makeEnumValues(Value0, Value1)

        EnumValueCounter<EnumType> __VA_ARGS__;
        return EnumTraitsHelper::makeEnumValues<EnumType>(__VA_ARGS__);
     */
    template <EnumValueType EnumType, typename IntType = typename std::underlying_type_t<EnumType>>
    struct EnumValueCounter
    {
        IntType value;

        EnumValueCounter(IntType v) noexcept :
            value(v)
        {
            s_enumValueCounter = value + 1;
        }

        EnumValueCounter() noexcept :
            EnumValueCounter(s_enumValueCounter)
        {
        }

        operator IntType() const noexcept
        {
            return value;
        }

    private:
        inline static IntType s_enumValueCounter = 0;
    };

    /**
     */
    struct MY_KERNEL_EXPORT EnumTraitsHelper
    {
        /**
            MY_DEFINE_ENUM_(MyEnum, Value0, Value1 = 10, Value3)
            #__VA_ARGS__ will maps to string "Value0, Value1 = 10, Value3".
            parseEnumDefinition - will split that definition to array of string_view = ["Value0", "Value1", "Value3"]
        */
        static void parseEnumDefinition(std::string_view enumDefinitionString, size_t itemCount, std::string_view* result);

        /**
         */
        static std::string_view toString(IEnumRuntimeInfo& enumInfo, int value);

        static Result<int> parse(IEnumRuntimeInfo& enumInfo, std::string_view str);

        template <EnumValueType T, size_t N>
        static auto makeIntValues(const std::array<T, N>& values)
        {
            std::array<int, N> intValues;
            for (size_t i = 0; i < N; ++i)
            {
                intValues[i] = static_cast<int>(values[i]);
            }

            return intValues;
        }

        template <EnumValueType EnumType, typename... Counters>
        static auto makeEnumValues(Counters... counter)
        {
            std::array<EnumType, sizeof...(Counters)> values{static_cast<EnumType>(counter.value)...};
            return values;
        }

        template <size_t N>
        static auto makeStrValues(std::string_view (&valueDefString)[N])
        {
            std::array<std::string_view, N> result;
            for (size_t i = 0; i < N; ++i)
            {
                result[i] = parseSingleEnumEntry(valueDefString[i]);
            }

            return result;
        }
    };

}  // namespace my::kernel_detail

namespace my
{
    template <EnumValueType EnumType>
    struct EnumTraits
    {
        static const IEnumRuntimeInfo& getRuntimeInfo()
        {
            // using ADL to find corresponding getEnumInternalInfo
            return getEnumInternalInfo(EnumType{});
        }

        static std::span<const EnumType> getValues()
        {
            decltype(auto) intern = getEnumInternalInfo(EnumType{});
            return {intern.enumValues, intern.itemCount};
        }

        static std::span<const std::string_view> getStrValues()
        {
            decltype(auto) intern = getEnumInternalInfo(EnumType{});
            return {intern.strValues, intern.itemCount};
        }

        static std::string_view toString(EnumType value)
        {
            return kernel_detail::EnumTraitsHelper::toString(getEnumInternalInfo(EnumType{}), static_cast<int>(value));
        }

        static Result<> parse(std::string_view str, EnumType& enumValue)
        {
            decltype(auto) intern = getEnumInternalInfo(EnumType{});
            Result<int> parseResult = kernel_detail::EnumTraitsHelper::parse(intern, str);
            NauCheckResult(parseResult);

            enumValue = static_cast<EnumType>(*parseResult);

            return ResultSuccess;
        }
    };
}  // namespace my

#define MY_DEFINE_ENUM(EnumType, EnumIntType, EnumName, ...)                                               \
    enum class EnumType : EnumIntType                                                                      \
    {                                                                                                      \
        __VA_ARGS__                                                                                        \
    };                                                                                                     \
                                                                                                           \
    [[maybe_unused]] MY_NOINLINE inline decltype(auto) getEnumInternalInfo(EnumType)                       \
    {                                                                                                      \
        using namespace my;                                                                                \
        using namespace my::kernel_detail;                                                                 \
        using namespace std::literals;                                                                     \
                                                                                                           \
        static auto s_enumValues = EXPR_Block                                                              \
        {                                                                                                  \
            EnumValueCounter<EnumType> __VA_ARGS__;                                                        \
            return EnumTraitsHelper::makeEnumValues<EnumType>(__VA_ARGS__);                                \
        };                                                                                                 \
                                                                                                           \
        static constexpr size_t ItemCount = s_enumValues.size();                                           \
        static const std::string_view s_enumDefinitionString = #__VA_ARGS__##sv;                           \
                                                                                                           \
        static auto s_enumStrValues = EXPR_Block                                                           \
        {                                                                                                  \
            std::array<std::string_view, ItemCount> parsedValues;                                          \
            EnumTraitsHelper::parseEnumDefinition(s_enumDefinitionString, ItemCount, parsedValues.data()); \
            return parsedValues;                                                                           \
        };                                                                                                 \
                                                                                                           \
        static auto s_enumIntValues = EnumTraitsHelper::makeIntValues(s_enumValues);                       \
                                                                                                           \
        struct EnumRuntimeInfo : EnumRuntimeInfoImpl                                                       \
        {                                                                                                  \
            const EnumType* const enumValues = s_enumValues.data();                                        \
                                                                                                           \
            EnumRuntimeInfo() :                                                                            \
                EnumRuntimeInfoImpl(EnumName, ItemCount, s_enumStrValues.data(), s_enumIntValues.data())   \
            {                                                                                              \
            }                                                                                              \
        };                                                                                                 \
                                                                                                           \
        static EnumRuntimeInfo s_enumData;                                                                 \
                                                                                                           \
        return (s_enumData);                                                                               \
    }                                                                                                      \
                                                                                                           \
    [[maybe_unused]] inline MY_NOINLINE std::string toString(EnumType value)                               \
    {                                                                                                      \
        const auto strValue = ::my::EnumTraits<EnumType>::toString(value);                                 \
        return {strValue.data(), strValue.size()};                                                         \
    }                                                                                                      \
                                                                                                           \
    [[maybe_unused]] inline MY_NOINLINE ::my::Result<> parse(std::string_view str, EnumType& value)        \
    {                                                                                                      \
        auto parseRes = ::my::EnumTraits<EnumType>::parse({str.data(), str.size()});                       \
        NauCheckResult(parseRes)                                                                           \
        value = *parseRes;                                                                                 \
                                                                                                           \
        return my::ResultSuccess;                                                                          \
    }

#define MY_DEFINE_ENUM_(EnumType, ...) MY_DEFINE_ENUM(EnumType, int, #EnumType, __VA_ARGS__)
