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
    class NativeTuple final : public ser_detail::NativeRuntimeValueBase<RuntimeReadonlyCollection>
    {
        using Base = ser_detail::NativeRuntimeValueBase<RuntimeReadonlyCollection>;
        using TupleType = std::decay_t<T>;

        MY_REFCOUNTED_CLASS(NativeTuple<T>, Base)

    public:
        static_assert(LikeTuple<TupleType>);
        static inline constexpr bool IsMutable = !std::is_const_v<std::remove_reference_t<T>>;
        static inline constexpr bool IsReference = std::is_lvalue_reference_v<T>;
        static inline constexpr size_t TupleSize = TupleValueOperations1<TupleType>::TupleSize;

        NativeTuple(T inTuple)
        requires(IsReference)
            :
            m_tuple(inTuple)
        {
        }

        template <typename U>
        NativeTuple(U&& inTuple)
        requires(!IsReference)
            :
            m_tuple(std::forward<U>(inTuple))
        {
        }

        bool isMutable() const override
        {
            return IsMutable;
        }

        size_t getSize() const override
        {
            return TupleSize;
        }

        RuntimeValuePtr getAt(size_t index) override
        {
            return getElementInternal(index,  std::make_index_sequence<TupleSize>{});
        }

        Result<> setAt(size_t index, const RuntimeValuePtr& value) override
        {
            MY_DEBUG_CHECK(value);
            return RuntimeValue::assign(getAt(index), value);
        }

    private:
        template <size_t... I>
        RuntimeValuePtr getElementInternal(size_t index, std::index_sequence<I...>) const
        {
            MY_DEBUG_CHECK(index < TupleSize, "Bad element index ({})", index);

            using ElementAccessorFunc = RuntimeValuePtr (*)(const NativeTuple<T>& self, std::add_lvalue_reference_t<T>);

            const std::array<ElementAccessorFunc, TupleSize> factories{wrapElementAsRuntimeValue<I>...};

            auto& f = factories[index];
            return f(*this, const_cast<TupleType&>(m_tuple));
        }

        template <size_t I>
        static RuntimeValuePtr wrapElementAsRuntimeValue(const NativeTuple<T>& self, std::add_lvalue_reference_t<T> container)
        {
            decltype(auto) el = TupleValueOperations1<TupleType>::template element<I>(container);
            return self.makeChildValue(makeValueRef(el));
        }

        T m_tuple;
    };

    template <typename T>
    class NativeUniformTuple final : public ser_detail::NativeRuntimeValueBase<RuntimeReadonlyCollection>
    {
        using Base = ser_detail::NativeRuntimeValueBase<RuntimeReadonlyCollection>;
        using TupleType = std::decay_t<T>;

        MY_REFCOUNTED_CLASS(NativeUniformTuple<T>, Base)

    public:
        static_assert(LikeUniformTuple<TupleType>);
        static inline constexpr bool IsMutable = !std::is_const_v<std::remove_reference_t<T>>;
        static inline constexpr bool IsReference = std::is_lvalue_reference_v<T>;
        static inline constexpr size_t TupleSize = UniformTupleValueOperations<TupleType>::TupleSize;

        NativeUniformTuple(T inTuple)
        requires(IsReference)
            :
            m_tuple(inTuple)
        {
        }

        template <typename U>
        NativeUniformTuple(U&& inTuple)
        requires(!IsReference)
            :
            m_tuple(std::forward<U>(inTuple))
        {
        }

        bool isMutable() const override
        {
            return IsMutable;
        }

        size_t getSize() const override
        {
            return TupleSize;
        }

        RuntimeValuePtr getAt(size_t index) override
        {
            decltype(auto) el = UniformTupleValueOperations<TupleType>::element(m_tuple, index);

            return this->makeChildValue(makeValueRef(el));
        }

        Result<> setAt(size_t index, const RuntimeValuePtr& value) override
        {
            MY_DEBUG_CHECK(value);
            MY_DEBUG_CHECK(index < TupleSize);

            decltype(auto) el = UniformTupleValueOperations<TupleType>::element(m_tuple, index);

            return RuntimeValue::assign(makeValueRef(el), value);
        }

    private:
        T m_tuple;
    };

}  // namespace my::ser_detail

namespace my
{
    template <LikeTuple T>
    Ptr<RuntimeReadonlyCollection> makeValueRef(T& tup, MemAllocator* allocator)
    {
        using Tuple = ser_detail::NativeTuple<T&>;
        return rtti::createInstanceWithAllocator<Tuple>(allocator, tup);
    }

    template <LikeTuple T>
    Ptr<RuntimeReadonlyCollection> makeValueRef(const T& tup, MemAllocator* allocator)
    {
        using Tuple = ser_detail::NativeTuple<const T&>;
        return rtti::createInstanceWithAllocator<Tuple>(allocator, tup);
    }

    template <LikeTuple T>
    Ptr<RuntimeReadonlyCollection> makeValueCopy(const T& tup, MemAllocator* allocator)
    {
        using Tuple = ser_detail::NativeTuple<T>;
        return rtti::createInstanceWithAllocator<Tuple>(allocator, tup);
    }

    template <LikeTuple T>
    Ptr<RuntimeReadonlyCollection> makeValueCopy(T&& tup, MemAllocator* allocator)
    {
        using Tuple = ser_detail::NativeTuple<T>;
        return rtti::createInstanceWithAllocator<Tuple>(allocator, std::move(tup));
    }

    template <LikeUniformTuple T>
    Ptr<RuntimeReadonlyCollection> makeValueRef(T& tup, MemAllocator* allocator)
    {
        using Tuple = ser_detail::NativeUniformTuple<T&>;
        return rtti::createInstanceWithAllocator<Tuple>(allocator, tup);
    }

    template <LikeUniformTuple T>
    Ptr<RuntimeReadonlyCollection> makeValueRef(const T& tup, MemAllocator* allocator)
    {
        using Tuple = ser_detail::NativeUniformTuple<const T&>;
        return rtti::createInstanceWithAllocator<Tuple>(allocator, tup);
    }

    template <LikeUniformTuple T>
    Ptr<RuntimeReadonlyCollection> makeValueCopy(const T& tup, MemAllocator* allocator)
    {
        using Tuple = ser_detail::NativeUniformTuple<T>;
        return rtti::createInstanceWithAllocator<Tuple>(allocator, tup);
    }

    template <LikeUniformTuple T>
    Ptr<RuntimeReadonlyCollection> makeValueCopy(T&& tup, MemAllocator* allocator)
    {
        using Tuple = ser_detail::NativeUniformTuple<T>;
        return rtti::createInstanceWithAllocator<Tuple>(allocator, std::move(tup));
    }
}  // namespace my
