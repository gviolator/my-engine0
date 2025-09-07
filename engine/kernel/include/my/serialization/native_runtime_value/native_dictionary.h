// #my_engine_source_file

#pragma once

#include <type_traits>

#include "my/rtti/rtti_impl.h"
#include "my/serialization/native_runtime_value/native_value_base.h"
#include "my/serialization/native_runtime_value/native_value_forwards.h"

namespace my::ser_detail
{
    template <typename T>
    class MapLikeNativeDictionary final : public ser_detail::NativeRuntimeValueBase<Dictionary>
    {
        using Base = ser_detail::NativeRuntimeValueBase<Dictionary>;
        using DictionaryType = std::decay_t<T>;

        MY_REFCOUNTED_CLASS(MapLikeNativeDictionary<T>, Base)

    public:
        static_assert(LikeStdMap<DictionaryType>);

        static inline constexpr bool IsMutable = !std::is_const_v<std::remove_reference_t<T>>;
        static inline constexpr bool IsReference = std::is_lvalue_reference_v<T>;

        MapLikeNativeDictionary(const DictionaryType& dict)
        requires(!IsReference)
            :
            m_dict(dict)
        {
        }

        MapLikeNativeDictionary(DictionaryType&& dict)
        requires(!IsReference)
            :
            m_dict(std::move(dict))
        {
        }

        MapLikeNativeDictionary(T dict)
        requires(IsReference)
            :
            m_dict(dict)
        {
        }

        bool isMutable() const override
        {
            return IsMutable;
        }

        size_t getSize() const override
        {
            return m_dict.size();
        }

        std::string_view getKey(size_t index) const override
        {
            auto head = m_dict.begin();
            std::advance(head, index);
            return std::string_view{head->first.data(), head->first.size()};
        }

        RuntimeValuePtr getValue(std::string_view key) override
        {
            auto iter = m_dict.find(typename DictionaryType::key_type{key.data(), key.size()});
            if(iter == m_dict.end())
            {
                return nullptr;
            }

            return this->makeChildValue(makeValueRef(iter->second));
        }

        bool containsKey(std::string_view key) const override
        {
            return m_dict.find(typename DictionaryType::key_type{key.data(), key.size()}) != m_dict.end();
        }

        void clear() override
        {
            if constexpr(IsMutable)
            {
                MY_DEBUG_FATAL(!this->hasChildren(), "Attempt to modify Runtime Collection while there is still referenced children");

                value_changes_scope;
                m_dict.clear();
            }
            else
            {
                MY_DEBUG_FAILURE("Attempt to modify non mutable value");
            }
        }

        Result<> setValue([[maybe_unused]] std::string_view key, [[maybe_unused]] const RuntimeValuePtr& newValue) override
        {
            if constexpr(IsMutable)
            {
                value_changes_scope;

                auto iter = m_dict.find(typename DictionaryType::key_type{key.data(), key.size()});
                if(iter == m_dict.end())
                {
                    auto [emplacedIter, emplaceOk] = m_dict.try_emplace(typename DictionaryType::key_type{key.data(), key.size()});
                    MY_DEBUG_ASSERT(emplaceOk);
                    iter = emplacedIter;
                }

                return RuntimeValue::assign(makeValueRef(iter->second), newValue);
            }
            else
            {
                MY_DEBUG_FAILURE("Attempt to modify non mutable value");
            }

            return {};
        }

        RuntimeValuePtr erase([[maybe_unused]] std::string_view key) override
        {
            if constexpr(IsMutable)
            {
                // value_changes_scope;
                // DictionaryValueOperations<DictionaryType>::
                MY_DEBUG_FAILURE("NativeDictionary::erase not implemented");
            }
            else
            {
                MY_DEBUG_FAILURE("Attempt to modify non mutable value");
            }

            return nullptr;
        }

    private:
        T m_dict;
    };
}  // namespace my::ser_detail

namespace my
{
    template <LikeStdMap T>
    Ptr<Dictionary> makeValueRef(T& dict, IAllocator* allocator)
    {
        using Dict = ser_detail::MapLikeNativeDictionary<T&>;

        return rtti::createInstanceWithAllocator<Dict>(allocator, dict);
    }

    template <LikeStdMap T>
    Ptr<Dictionary> makeValueRef(const T& dict, IAllocator* allocator)
    {
        using Dict = ser_detail::MapLikeNativeDictionary<const T&>;

        return rtti::createInstanceWithAllocator<Dict>(allocator, dict);
    }

    template <LikeStdMap T>
    Ptr<Dictionary> makeValueCopy(const T& dict, IAllocator* allocator)
    {
        using Dict = ser_detail::MapLikeNativeDictionary<T>;

        return rtti::createInstanceWithAllocator<Dict>(allocator, dict);
    }

    template <LikeStdMap T>
    Ptr<Dictionary> makeValueCopy(T&& dict, IAllocator* allocator)
    {
        using Dict = ser_detail::MapLikeNativeDictionary<T>;

        return rtti::createInstanceWithAllocator<Dict>(allocator, std::move(dict));
    }

}  // namespace my
