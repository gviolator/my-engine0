// #my_engine_source_file

#pragma once

#include <filesystem>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <type_traits>

#include "my/diag/logging.h"
#include "my/io/stream.h"
#include "my/memory/mem_allocator.h"
#include "my/memory/runtime_stack.h"
#include "my/rtti/type_info.h"
#include "my/serialization/runtime_value_builder.h"
#include "my/utils/functor.h"

namespace my
{
    /**
        @brief Application global properties access
     */
    struct MY_ABSTRACT_TYPE PropertyContainer
    {
        MY_TYPEID(my::PropertyContainer)

        using ModificationLock = std::unique_lock<std::shared_mutex>;
        using ReadOnlyLock = std::shared_lock<std::shared_mutex>;
        using VariableResolverCallback = Functor<std::optional<std::string>(std::string_view)>;

        virtual ~PropertyContainer() = default;

        /**
            @brief checks the property at path exists within dictionary
            @param path Property path.
        */
        virtual bool contains(std::string_view path) const = 0;

        /**
            @brief get read-only property at path as runtime value
            @param path Property path. Can be compound: with sections separated by '/': "app/section_0/prop_1"
            @return value as RuntimeValue or null if property does not exists
        */
        virtual RuntimeValuePtr getRead(std::string_view path, ReadOnlyLock& lock, IMemAllocator* allocator = nullptr) const = 0;

        /**
            @brief get property as modifiable runtime value
            @param path Property path.
            @param lock the synchronization mutex
        */
        virtual Result<RuntimeValuePtr> getModify(std::string_view path, ModificationLock& lock, IMemAllocator* allocator = nullptr) = 0;


        /**
            @brief setting property value at path
                Existing primitive value (numbers, strings, booleans) in path will be reset to the new one,
            but collections (arrays and objects/dictionaries) will be merged.

            @param path Property path.
        */
        virtual Result<> set(std::string_view path, RuntimeValuePtr value) = 0;

        /**
            @brief applies all the properties from values
        */
        virtual Result<> mergeWithValue(const RuntimeValue& value) = 0;

        /**
         */
        virtual void addVariableResolver(std::string_view kind, VariableResolverCallback resolver) = 0;

        /**
            @brief getting typed value
         */
        template <RuntimeValueRepresentable T>
        requires(std::is_default_constructible_v<T>)
        std::optional<T> getValue(std::string_view path);

        /**
            @brief setting typed value
         */
        template <RuntimeValueRepresentable T>
        Result<> setValue(std::string_view path, const T& value);
    };

    template <RuntimeValueRepresentable T>
    requires(std::is_default_constructible_v<T>)
    std::optional<T> PropertyContainer::getValue(std::string_view path)
    {
        rtstack_scope;

        ReadOnlyLock lock;
        RuntimeValuePtr value = getRead(path, lock, get_rt_stack_allocator_ptr());
        if (!value)
        {
            return std::nullopt;
        }

        T resultValue;
        if (const Result<> applyResult = runtimeValueApply(resultValue, value); !applyResult)
        {
            mylog_warn("Fail to apply property value at path({}):{}", path, applyResult.getError()->getMessage());
            return std::nullopt;
        }

        return resultValue;
    }

    template <RuntimeValueRepresentable T>
    Result<> PropertyContainer::setValue(std::string_view path, const T& value)
    {
        rtstack_scope;

        return set(path, makeValueRef(value));
    }

    /**
        @brief reads and parses a stream, then applies all the properties it retrieves to the properties dictionary.
    */
    Result<> merge_properties_from_stream(PropertyContainer& properties, io::IStream& stream, std::string_view contentType = "application/json");

    /**
        @brief reads and parses a file, then applies all the properties it retrieves to the properties dictionary.
    */
    Result<> merge_properties_from_file(PropertyContainer& properties, const std::filesystem::path& filePath, std::string_view contentType = {});

    /**
        @brief Serialize properties content into the specified stream.
     */
    void dump_properties_to_stream(PropertyContainer& properties, io::IStream& stream, std::string_view contentType = "application/json");

    /**
        @brief Serialize properties content into the string.
     */
    std::string dump_properties_to_string(PropertyContainer& properties, std::string_view contentType = "application/json");

}  // namespace my
