// #my_engine_source_file

#pragma once

#include <type_traits>

#include "my/rtti/rtti_impl.h"
#include "my/serialization/native_runtime_value/native_value_base.h"
#include "my/serialization/native_runtime_value/native_value_forwards.h"

namespace my::ser_detail
{
    /**
     */
    template <typename T>
    class VectorLikeNativeCollection final : public ser_detail::NativeRuntimeValueBase<RuntimeCollection>
    {
        using Base = ser_detail::NativeRuntimeValueBase<RuntimeCollection>;
        using ContainerType = std::decay_t<T>;

        MY_REFCOUNTED_CLASS(VectorLikeNativeCollection<T>, Base)

    public:
        static_assert(LikeStdVector<ContainerType>);
        static constexpr bool IsMutable = !std::is_const_v<std::remove_reference_t<T>>;
        static constexpr bool IsReference = std::is_lvalue_reference_v<T>;

        VectorLikeNativeCollection(const ContainerType& inCollection)
        requires(!IsReference)
            :
            m_collection(inCollection)
        {
        }

        VectorLikeNativeCollection(ContainerType&& inCollection)
        requires(!IsReference)
            :
            m_collection(std::move(inCollection))
        {
        }

        VectorLikeNativeCollection(T inCollection)
        requires(IsReference)
            :
            m_collection(inCollection)
        {
        }

        bool isMutable() const override
        {
            return IsMutable;
        }

        size_t getSize() const override
        {
            return m_collection.size();
        }

        RuntimeValuePtr getAt(size_t index) override
        {
            MY_DEBUG_CHECK(index < m_collection.size());

            return this->makeChildValue(makeValueRef(m_collection[index]));
        }

        Result<> setAt(size_t index, const RuntimeValuePtr& value) override
        {
            MY_DEBUG_CHECK(value);
            MY_DEBUG_CHECK(index < m_collection.size());

            return RuntimeValue::assign(makeValueRef(m_collection[index]), value);
        }

        void clear() override
        {
            if constexpr(IsMutable)
            {
                MY_DEBUG_FATAL(!this->hasChildren(), "Attempt to modify Runtime Collection while there is still referenced children");

                value_changes_scope;
                m_collection.clear();
            }
            else
            {
                MY_DEBUG_FAILURE("Attempt to modify non mutable array value");
            }
        }

        void reserve(size_t capacity) override
        {
            if constexpr(IsMutable)
            {
                MY_DEBUG_FATAL(!this->hasChildren(), "Attempt to modify Runtime Collection while there is still referenced children");
                m_collection.reserve(capacity);
            }
            else
            {
                MY_DEBUG_FAILURE("Can not reserve for non mutable array");
            }
        }

        Result<> append(const RuntimeValuePtr& value) override
        {
            if constexpr(!IsMutable)
            {
                MY_DEBUG_FAILURE("Attempt to modify non mutable array");
                return MakeError("Attempt to modify non mutable value");
            }
            else
            {
                MY_DEBUG_FATAL(!this->hasChildren(), "Attempt to modify Runtime Collection while there is still referenced children");

                value_changes_scope;

                m_collection.emplace_back();
                decltype(auto) newElement = m_collection.back();
                return RuntimeValue::assign(makeValueRef(newElement), value);
            }
        }

    private:
        T m_collection;
    };

    /**
     */
    template <typename T>
    class ListLikeNativeCollection final : public ser_detail::NativeRuntimeValueBase<RuntimeCollection>
    {
        using Base = ser_detail::NativeRuntimeValueBase<RuntimeCollection>;
        using ContainerType = std::decay_t<T>;

        MY_REFCOUNTED_CLASS(ListLikeNativeCollection<T>, Base)

    public:
        static_assert(LikeStdList<ContainerType>);
        static constexpr bool IsMutable = !std::is_const_v<std::remove_reference_t<T>>;
        static constexpr bool IsReference = std::is_lvalue_reference_v<T>;

        ListLikeNativeCollection(const ContainerType& inCollection)
        requires(!IsReference)
            :
            m_collection(inCollection)
        {
        }

        ListLikeNativeCollection(ContainerType&& inCollection)
        requires(!IsReference)
            :
            m_collection(std::move(inCollection))
        {
        }

        ListLikeNativeCollection(T inCollection)
        requires(IsReference)
            :
            m_collection(inCollection)
        {
        }

        bool isMutable() const override
        {
            return IsMutable;
        }

        size_t getSize() const override
        {
            return m_collection.size();
        }

        RuntimeValuePtr getAt(size_t index) override
        {
            MY_DEBUG_CHECK(index < m_collection.size(), "[{}], size():{}", index, m_collection.size());

            auto element = m_collection.begin();
            std::advance(element, index);

            return this->makeChildValue(makeValueRef(*element));
        }

        Result<> setAt(size_t index, const RuntimeValuePtr& value) override
        {
            MY_DEBUG_CHECK(value);
            MY_DEBUG_CHECK(index < m_collection.size(), "[{}], size():{}", index, m_collection.size());

            auto element = m_collection.begin();
            std::advance(element, index);

            return RuntimeValue::assign(makeValueRef(*element), value);
        }

        void clear() override
        {
            if constexpr(IsMutable)
            {
                MY_DEBUG_FATAL(!this->hasChildren(), "Attempt to modify Runtime Collection while there is still referenced children");

                value_changes_scope;
                m_collection.clear();
            }
            else
            {
                MY_DEBUG_FAILURE("Attempt to modify non mutable array value");
            }
        }

        void reserve([[maybe_unused]] size_t) override
        {
            if constexpr(!IsMutable)
            {
                MY_DEBUG_FAILURE("Can not reserve for non mutable array");
            }
        }

        Result<> append(const RuntimeValuePtr& value) override
        {
            if constexpr(!IsMutable)
            {
                MY_DEBUG_FAILURE("Attempt to modify non mutable value");
                return MakeError("Attempt to modify non mutable value");
            }
            else
            {
                value_changes_scope;

                // std::list<T>::emplace_back() returns nothing.
                m_collection.emplace_back();
                decltype(auto) newElement = m_collection.back();
                return RuntimeValue::assign(makeValueRef(newElement), value);
            }
        }

    private:

        T m_collection;
    };


    template <typename T>
    class SetLikeNativeCollection final : public ser_detail::NativeRuntimeValueBase<RuntimeCollection>
    {
        using Base = ser_detail::NativeRuntimeValueBase<RuntimeCollection>;
        using ContainerType = std::decay_t<T>;

        MY_REFCOUNTED_CLASS(SetLikeNativeCollection<T>, Base)

    public:
        static_assert(LikeSet<ContainerType>);
        static constexpr bool IsMutable = !std::is_const_v<std::remove_reference_t<T>>;
        static constexpr bool IsReference = std::is_lvalue_reference_v<T>;

        SetLikeNativeCollection(const ContainerType& inCollection)
        requires(!IsReference)
            :
            m_collection(inCollection)
        {
        }

        SetLikeNativeCollection(ContainerType&& inCollection)
        requires(!IsReference)
            :
            m_collection(std::move(inCollection))
        {
        }

        SetLikeNativeCollection(T inCollection)
        requires(IsReference)
            :
            m_collection(inCollection)
        {
        }

        bool isMutable() const override
        {
            return IsMutable;
        }

        size_t getSize() const override
        {
            return m_collection.size();
        }

        RuntimeValuePtr getAt(size_t index) override
        {
            MY_DEBUG_CHECK(index < m_collection.size(), "[{}], size():{}", index, m_collection.size());

            auto element = m_collection.begin();
            std::advance(element, index);

            return this->makeChildValue(makeValueRef(*element));
        }

        Result<> setAt(size_t index, const RuntimeValuePtr& value) override
        {
            MY_DEBUG_CHECK(value);
            MY_DEBUG_CHECK(index < m_collection.size(), "[{}], size():{}", index, m_collection.size());

            auto element = m_collection.begin();
            std::advance(element, index);

            return RuntimeValue::assign(makeValueRef(*element), value);
        }

        void clear() override
        {
            if constexpr(IsMutable)
            {
                MY_DEBUG_FATAL(!this->hasChildren(), "Attempt to modify Runtime Collection while there is still referenced children");

                value_changes_scope;
                m_collection.clear();
            }
            else
            {
                MY_DEBUG_FAILURE("Attempt to modify non mutable array value");
            }
        }

        void reserve([[maybe_unused]] size_t) override
        {
            if constexpr(!IsMutable)
            {
                MY_DEBUG_FAILURE("Can not reserve for non mutable array");
            }
        }

        Result<> append(const RuntimeValuePtr& value) override
        {
            if constexpr(!IsMutable)
            {
                MY_DEBUG_FAILURE("Attempt to modify non mutable value");
                return MakeError("Attempt to modify non mutable value");
            }
            else
            {
                value_changes_scope;

                typename ContainerType::value_type newElement;
                CheckResult(RuntimeValue::assign(makeValueRef(newElement), value))
                [[maybe_unused]] auto [iter, emplaceOk] = m_collection.emplace(std::move(newElement));
                MY_DEBUG_CHECK(emplaceOk, "Fail to emplace element (expects that collection holds only unique values)");
                
                return emplaceOk ? ResultSuccess : MakeError("Fail to append (unique) value");
            }
        }

    private:

        T m_collection;
    };
}  // namespace my::ser_detail

namespace my
{

    template <LikeStdVector T>
    Ptr<RuntimeCollection> makeValueRef(T& collection, MemAllocator* allocator)
    {
        using Collection = ser_detail::VectorLikeNativeCollection<T&>;

        return rtti::createInstanceWithAllocator<Collection>(allocator, collection);
    }

    template <LikeStdVector T>
    Ptr<RuntimeCollection> makeValueRef(const T& collection, MemAllocator* allocator)
    {
        using Collection = ser_detail::VectorLikeNativeCollection<const T&>;

        return rtti::createInstanceWithAllocator<Collection>(allocator, collection);
    }

    template <LikeStdVector T>
    Ptr<RuntimeCollection> makeValueCopy(const T& collection, MemAllocator* allocator)
    {
        using Collection = ser_detail::VectorLikeNativeCollection<T>;

        return rtti::createInstanceWithAllocator<Collection>(allocator, collection);
    }

    template <LikeStdVector T>
    Ptr<RuntimeCollection> makeValueCopy(T&& collection, MemAllocator* allocator)
    {
        using Collection = ser_detail::VectorLikeNativeCollection<T>;

        return rtti::createInstanceWithAllocator<Collection>(std::move(allocator), std::move(collection));
    }

    template <LikeStdList T>
    Ptr<RuntimeCollection> makeValueRef(T& collection, MemAllocator* allocator)
    {
        using Collection = ser_detail::ListLikeNativeCollection<T&>;

        return rtti::createInstanceWithAllocator<Collection>(allocator, collection);
    }

    template <LikeStdList T>
    Ptr<RuntimeCollection> makeValueRef(const T& collection, MemAllocator* allocator)
    {
        using Collection = ser_detail::ListLikeNativeCollection<const T&>;

        return rtti::createInstanceWithAllocator<Collection>(allocator, collection);
    }

    template <LikeStdList T>
    Ptr<RuntimeCollection> makeValueCopy(const T& collection, MemAllocator* allocator)
    {
        using Collection = ser_detail::ListLikeNativeCollection<T>;

        return rtti::createInstanceWithAllocator<Collection>(allocator, collection);
    }

    template <LikeStdList T>
    Ptr<RuntimeCollection> makeValueCopy(T&& collection, MemAllocator* allocator)
    {
        using Collection = ser_detail::ListLikeNativeCollection<T>;

        return rtti::createInstanceWithAllocator<Collection>(std::move(allocator), std::move(collection));
    }

    template <LikeSet T>
    Ptr<RuntimeCollection> makeValueRef(T& collection, MemAllocator* allocator)
    {
        using Collection = ser_detail::SetLikeNativeCollection<T&>;

        return rtti::createInstanceWithAllocator<Collection>(allocator, collection);
    }

    template <LikeSet T>
    Ptr<RuntimeCollection> makeValueRef(const T& collection, MemAllocator* allocator)
    {
        using Collection = ser_detail::SetLikeNativeCollection<const T&>;

        return rtti::createInstanceWithAllocator<Collection>(allocator, collection);
    }

    template <LikeSet T>
    Ptr<RuntimeCollection> makeValueCopy(const T& collection, MemAllocator* allocator)
    {
        using Collection = ser_detail::SetLikeNativeCollection<T>;

        return rtti::createInstanceWithAllocator<Collection>(allocator, collection);
    }

    template <LikeSet T>
    Ptr<RuntimeCollection> makeValueCopy(T&& collection, MemAllocator* allocator)
    {
        using Collection = ser_detail::SetLikeNativeCollection<T>;

        return rtti::createInstanceWithAllocator<Collection>(std::move(allocator), std::move(collection));
    }    
}  // namespace my
