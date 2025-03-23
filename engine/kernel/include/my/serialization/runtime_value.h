// #my_engine_source_header
#pragma once
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "my/diag/check.h"
#include "my/kernel/kernel_config.h"
#include "my/memory/mem_allocator.h"
#include "my/rtti/ptr.h"
#include "my/rtti/rtti_object.h"
#include "my/utils/numeric_cast.h"
#include "my/utils/result.h"
#include "my/utils/typed_flag.h"
#include "my/utils/uni_ptr.h"


namespace my
{
    /**
     */
    enum class ValueAssignOption
    {
        MergeCollection = FlagValue(0)
    };

    MY_DEFINE_TYPED_FLAG(ValueAssignOption)

    using RuntimeValuePtr = Ptr<struct RuntimeValue>;

    /**

    */
    struct MY_ABSTRACT_TYPE RuntimeValue : IRefCounted
    {
        MY_INTERFACE(my::RuntimeValue, IRttiObject)

        MY_KERNEL_EXPORT
        static Result<> assign(RuntimeValuePtr dst, RuntimeValuePtr src, ValueAssignOptionFlag = {});

        virtual bool isMutable() const = 0;
    };


    /**

    */
    struct MY_ABSTRACT_TYPE RuntimeValueRef : RuntimeValue
    {
        MY_INTERFACE(my::RuntimeValueRef, RuntimeValue)

        MY_KERNEL_EXPORT
        static my::Ptr<RuntimeValueRef> create(RuntimeValuePtr&, IMemAllocator* = nullptr);

        MY_KERNEL_EXPORT
        static my::Ptr<RuntimeValueRef> create(std::reference_wrapper<const RuntimeValuePtr&>, IMemAllocator* = nullptr);

        virtual void setValue(RuntimeValuePtr) = 0;
 
        virtual RuntimeValuePtr getValue() const = 0;
    };

    /**
     */
    struct MY_ABSTRACT_TYPE RuntimePrimitiveValue : RuntimeValue
    {
        MY_INTERFACE(my::RuntimePrimitiveValue, RuntimeValue)
    };

    /**
     */
    struct MY_ABSTRACT_TYPE RuntimeStringValue : RuntimePrimitiveValue
    {
        MY_INTERFACE(my::RuntimeStringValue, RuntimePrimitiveValue)

        /*
         */
        virtual Result<> setString(std::string_view) = 0;

        /*
         */
        virtual std::string getString() const = 0;
    };

    /**
     */
    struct MY_ABSTRACT_TYPE RuntimeIntegerValue : RuntimePrimitiveValue
    {
        MY_INTERFACE(my::RuntimeIntegerValue, RuntimePrimitiveValue)

        /*
         */
        virtual bool isSigned() const = 0;

        /*
         */
        virtual size_t getBitsCount() const = 0;

        /*
         */
        virtual void setInt64(int64_t) = 0;

        /*
         */
        virtual void setUint64(uint64_t) = 0;

        /*
         */
        virtual int64_t getInt64() const = 0;

        /*
         */
        virtual uint64_t getUint64() const = 0;

        template <typename T>
        inline void set(T value)
        requires(std::is_integral_v<T>)
        {
            if constexpr (std::is_signed_v<T>)
            {
                this->setInt64(static_cast<int64_t>(value));
            }
            else
            {
                this->setUint64(static_cast<uint64_t>(value));
            }
        }

        template <typename T>
        T get() const
        requires(std::is_integral_v<T>)
        {
            if constexpr (std::is_signed_v<T>)
            {
                const int64_t value = this->getInt64();
                return my::numeric_cast<T>(value);
            }
            else
            {
                const uint64_t value = this->getUint64();
                return my::numeric_cast<T>(value);
            }
        }
    };

    /**
     */
    struct MY_ABSTRACT_TYPE RuntimeFloatValue : RuntimePrimitiveValue
    {
        MY_INTERFACE(my::RuntimeFloatValue, RuntimePrimitiveValue)

        virtual size_t getBitsCount() const = 0;

        virtual void setDouble(double) = 0;

        virtual void setSingle(float) = 0;

        virtual double getDouble() const = 0;

        virtual float getSingle() const = 0;

        template <typename T>
        T get() const
        requires(std::is_arithmetic_v<T>)
        {
            return static_cast<T>(this->getDouble());
        }

        template <typename T>
        void set(T value)
        requires(std::is_arithmetic_v<T>)
        {
            return this->setDouble(static_cast<double>(value));
        }
    };

    /**
     */
    struct MY_ABSTRACT_TYPE RuntimeBooleanValue : RuntimePrimitiveValue
    {
        MY_INTERFACE(my::RuntimeBooleanValue, RuntimePrimitiveValue)

        using Ptr = my::Ptr<RuntimeBooleanValue>;

        virtual void setBool(bool) = 0;

        virtual bool getBool() const = 0;
    };

    /**

    */
    struct MY_ABSTRACT_TYPE RuntimeOptionalValue : RuntimeValue
    {
        MY_INTERFACE(my::RuntimeOptionalValue, RuntimeValue)

        using Ptr = my::Ptr<RuntimeOptionalValue>;

        virtual bool hasValue() const = 0;

        virtual RuntimeValuePtr getValue() = 0;

        virtual Result<> setValue(RuntimeValuePtr value = nullptr) = 0;

        inline void reset()
        {
            [[maybe_unused]] auto res = this->setValue(nullptr);
            MY_DEBUG_CHECK(res);
            if (res.isError())
            {
                // Halt(std::format("Fail to reset optional:", res.getError()->Message()));
            }
        }
    };

    /**
     */
    struct MY_ABSTRACT_TYPE RuntimeReadonlyCollection : virtual RuntimeValue
    {
        MY_INTERFACE(my::RuntimeReadonlyCollection, RuntimeValue)

        using Ptr = my::Ptr<RuntimeReadonlyCollection>;

        virtual size_t getSize() const = 0;

        virtual RuntimeValuePtr getAt(size_t index) = 0;

        virtual Result<> setAt(size_t index, const RuntimeValuePtr& value) = 0;

        RuntimeValuePtr operator[](size_t index)
        {
            return getAt(index);
        }
    };

    /**
     */
    struct MY_ABSTRACT_TYPE RuntimeCollection : virtual RuntimeReadonlyCollection
    {
        MY_INTERFACE(my::RuntimeCollection, RuntimeReadonlyCollection)

        using Ptr = my::Ptr<RuntimeCollection>;

        virtual void clear() = 0;

        virtual void reserve(size_t) = 0;

        virtual Result<> append(const RuntimeValuePtr&) = 0;
    };

    /**
     */
    struct MY_ABSTRACT_TYPE RuntimeReadonlyDictionary : virtual RuntimeValue
    {
        MY_INTERFACE(my::RuntimeReadonlyDictionary, RuntimeValue)

        using Ptr = my::Ptr<RuntimeReadonlyDictionary>;

        virtual size_t getSize() const = 0;

        virtual std::string_view getKey(size_t index) const = 0;

        virtual RuntimeValuePtr getValue(std::string_view) = 0;

        virtual Result<> setValue(std::string_view, const RuntimeValuePtr& value) = 0;

        virtual bool containsKey(std::string_view) const = 0;

        RuntimeValuePtr operator[](std::string_view key)
        {
            return getValue(key);
        }

        inline std::pair<std::string_view, RuntimeValuePtr> operator[](size_t index)
        {
            auto key = getKey(index);
            return {key, getValue(key)};
        }
    };

    /**
     *
     */
    struct MY_ABSTRACT_TYPE RuntimeDictionary : virtual RuntimeReadonlyDictionary
    {
        MY_INTERFACE(my::RuntimeDictionary, RuntimeReadonlyDictionary)

        using Ptr = my::Ptr<RuntimeDictionary>;

        virtual void clear() = 0;

        virtual RuntimeValuePtr erase(std::string_view) = 0;
    };

    /**
        Generalized object runtime representation.
    */
    struct MY_ABSTRACT_TYPE RuntimeObject : virtual RuntimeReadonlyDictionary
    {
        MY_INTERFACE(my::RuntimeObject, RuntimeReadonlyDictionary)

        using Ptr = my::Ptr<RuntimeObject>;

        struct FieldInfo
        {
        };

        virtual std::optional<FieldInfo> findFieldInfo(std::string_view) const = 0;

        inline Result<> setFieldValue(std::string_view key, const RuntimeValuePtr& value)
        {
            return setValue(key, value);
        }
    };

    /**
     */
    // struct MY_ABSTRACT_TYPE RuntimeCustomValue : virtual RuntimeValue
    // {
    //     MY_INTERFACE(my::RuntimeCustomValue, RuntimeValue)

    //     virtual Result<> restore(const RuntimeValue&) = 0;

    //     virtual Result<> store(RuntimeValue&) const = 0;
    // };

    /**
     */
    struct MY_ABSTRACT_TYPE RuntimeNativeValue : virtual RuntimeValue  // virtual RuntimeCustomValue
    {
        MY_INTERFACE(my::RuntimeNativeValue, RuntimeValue)

        virtual const rtti::TypeInfo* getValueTypeInfo() const = 0;

        virtual const void* getReadonlyValuePtr() const = 0;

        virtual void* getValuePtr() = 0;

        template <typename T>
        const T& getReadonlyRef() const
        {
            if constexpr (rtti::HasTypeInfo<T>)
            {
                [[maybe_unused]]
                const auto* const type = getValueTypeInfo();
                MY_DEBUG_CHECK(*type == rtti::getTypeInfo<T>());
            }

            const void* const valuePtr = getReadonlyValuePtr();
            MY_DEBUG_CHECK(valuePtr);

            return *reinterpret_cast<const T*>(valuePtr);
        }

        template <typename T>
        T& getRef()
        {
            if constexpr (rtti::HasTypeInfo<T>)
            {
                [[maybe_unused]]
                const auto* const type = getValueTypeInfo();
                MY_DEBUG_CHECK(*type == rtti::getTypeInfo<T>());
            }

            void* const valuePtr = getValuePtr();
            MY_DEBUG_CHECK(valuePtr);

            return *reinterpret_cast<T*>(valuePtr);
        }
    };

}  // namespace my
