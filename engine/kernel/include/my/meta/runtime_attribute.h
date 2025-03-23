// #my_engine_source_header
#pragma once

#include <type_traits>
#include <vector>
#include <optional>

#include "my/diag/logging.h"
#include "my/kernel/kernel_config.h"
#include "my/meta/attribute.h"
#include "my/serialization/runtime_value_builder.h"
#include "my/utils/tuple_utility.h"
#include "my/utils/type_tag.h"

namespace my::meta
{
    /**
     */
    struct MY_ABSTRACT_TYPE IRuntimeAttributeContainer
    {
        virtual ~IRuntimeAttributeContainer() = default;

        /**
            @brief getting attribute unique keys count
            Does not count multiple values for the same key.
         */
        virtual size_t getSize() const = 0;

        /**
            @brief checks that an attribute with a given key exists in a collection
         */
        virtual bool containsAttribute(std::string_view key) const = 0;

        /**
            @brief get key name at index
         */
        virtual std::string_view getKey(size_t index) const = 0;

        /**
            @brief getting value associated with a key.
            If there is multiple attributes for that key - only first will be returned
         */
        virtual RuntimeValue::Ptr getValue(std::string_view key) const = 0;

        /**
            @brief getting all values associated with a key.
         */
        virtual std::vector<RuntimeValue::Ptr> getAllValues(std::string_view key) const = 0;

        /**
         */
        template <std::derived_from<Attribute> Key>
        bool contains() const
        {
            return containsAttribute(Key{}.strValue);
        }

        /**
         */
        template <std::derived_from<Attribute> Key, typename T>
        requires(HasRuntimeValueRepresentation<T> && std::is_default_constructible_v<T>)
        std::optional<T> get() const
        {
            RuntimeValue::Ptr value = getValue(Key{}.strValue);
            if (!value)
            {
                return std::nullopt;
            }

            T result;
            Result<> assignResult = runtimeValueApply(result, value);
            if (!assignResult)
            {
                MY_LOG_ERROR("Fail to assign attribute ({}) value:{}", Key{}.strValue, assignResult.getError()->getMessage());
                return std::nullopt;
            }

            return result;
        }

        template <std::derived_from<Attribute> Key>
        std::vector<RuntimeValue::Ptr> getAll() const
        {
            return getAllValues(Key{}.strValue);
        }
    };

    /**
     */
    class MY_KERNEL_EXPORT RuntimeAttributeContainer final
        : public IRuntimeAttributeContainer
    {
    public:
        template <typename T>
        RuntimeAttributeContainer(TypeTag<T>);

        RuntimeAttributeContainer() = delete;
        RuntimeAttributeContainer(const RuntimeAttributeContainer&) =
            default;
        RuntimeAttributeContainer(RuntimeAttributeContainer&&) = default;

        RuntimeAttributeContainer&
        operator=(const RuntimeAttributeContainer&) = default;
        RuntimeAttributeContainer&
        operator=(RuntimeAttributeContainer&&) = default;

        size_t getSize() const override;

        bool containsAttribute(std::string_view key) const override;

        std::string_view getKey(size_t index) const override;

        RuntimeValue::Ptr getValue(std::string_view key) const override;

        std::vector<RuntimeValue::Ptr> getAllValues(std::string_view key) const override;

    private:
        using AttributeEntry = std::pair<std::string_view, RuntimeValue::Ptr>;

        void setupUniqueKeys();

        std::vector<AttributeEntry> m_attributes;
        std::vector<std::string_view> m_uniqueKeys;
    };

    template <typename T>
    RuntimeAttributeContainer::RuntimeAttributeContainer(TypeTag<T>)
    {
        auto attributes = meta::getClassAllAttributes<T>();

        constexpr size_t AttributesCount =
            std::tuple_size_v<decltype(attributes)>;
        if constexpr (AttributesCount > 0)
        {
            m_attributes.reserve(AttributesCount);
            TupleUtils::forEach(
                attributes, [this]<typename Key, typename Value>(
                                AttributeField<Key, Value>& field)
            {
                if constexpr (HasRuntimeValueRepresentation<Value>)
                {
                    // attribute must have defined string value.
                    if (const Key key{}; !key.strValue.empty())
                    {
                        RuntimeValue::Ptr value =
                            makeValueCopy(std::move(field.value));
                        m_attributes.emplace_back(key.strValue, std::move(value));
                    }
                }
            });

            setupUniqueKeys();
        }
    }

    template <typename T>
    RuntimeAttributeContainer makeRuntimeAttributeContainer()
    {
        return RuntimeAttributeContainer(TypeTag<T>{});
    }

}  // namespace my::meta
