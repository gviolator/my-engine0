// #my_engine_source_header


#pragma once

#include <type_traits>
#include <typeinfo>

#include "my/diag/check.h"
#include "my/rtti/type_info.h"
#include "my/utils/preprocessor.h"

#define MY_INTERFACE(TypeName, ...) \
    MY_TYPEID(TypeName)             \
    MY_CLASS_BASE(__VA_ARGS__)

namespace my
{
    struct IRefCounted;

    struct IWeakRef;

    /**
     */
    struct MY_ABSTRACT_TYPE IRttiObject
    {
        MY_TYPEID(my::IRttiObject);

        virtual ~IRttiObject() = default;
        virtual bool is(const rtti::TypeInfo&) const noexcept = 0;
        virtual void* as(const rtti::TypeInfo&) noexcept = 0;
        virtual const void* as(const rtti::TypeInfo&) const noexcept = 0;

        IRttiObject& operator=(const IRttiObject&) = default;

        template <typename T>
        T as() const
            requires std::is_pointer_v<T>
        {
            using Interface = std::remove_pointer_t<T>;
            static_assert(std::is_const_v<Interface>, "Attempt to cast through constant instance. const T must be explicitly specified: use 'as<const T*>'");

            const void* const ptr = this->as(rtti::getTypeInfo<std::remove_const_t<Interface>>());
            return reinterpret_cast<T>(ptr);
        }

        template <typename T>
        T as()
            requires std::is_pointer_v<T>
        {
            using Interface = std::remove_pointer_t<T>;

            void* const ptr = this->as(rtti::getTypeInfo<std::remove_const_t<Interface>>());
            return reinterpret_cast<T>(ptr);
        }

        template <typename T>
        T as()
            requires std::is_reference_v<T>
        {
            using Interface = std::remove_reference_t<T>;
            void* const ptr = this->as(rtti::getTypeInfo<std::remove_const_t<Interface>>());
            MY_DEBUG_CHECK(ptr);

            return *reinterpret_cast<Interface*>(ptr);
        }

        template <typename T>
        T as() const
            requires std::is_reference_v<T>
        {
            using Interface = std::remove_reference_t<T>;
            static_assert(std::is_const_v<Interface>, "Attempt to cast through constant instance. const T must be explicitly specified: use 'as<const T&>'");

            const void* const ptr = this->as(rtti::getTypeInfo<std::remove_const_t<Interface>>());
            MY_DEBUG_CHECK(ptr);

            return *reinterpret_cast<Interface*>(ptr);
        }

        template <typename T, std::enable_if_t<!std::is_reference_v<T> && !std::is_pointer_v<T>, int> = 0>
        T as() const
        {
            constexpr bool NotPointerOrReference = !(std::is_reference_v<T> || std::is_pointer_v<T>);
            static_assert(NotPointerOrReference, "Type for 'as' must be pointer or reference: use 'as<T*>' or 'as<T&>'");
            return reinterpret_cast<T*>(nullptr);
        }

        template <typename T>
        bool is() const
        {
            static_assert(!(std::is_reference_v<T> || std::is_pointer_v<T> || std::is_const_v<T>), "Invalid requested type");

            if constexpr(rtti::HasTypeInfo<T>)
            {
                return this->is(rtti::getTypeInfo<T>());
            }
            else
            {
                return false;
            }
        }
    };

    /*!
        Ref counted description
    */
    struct MY_ABSTRACT_TYPE IRefCounted : virtual IRttiObject
    {
        MY_INTERFACE(my::IRefCounted, IRttiObject)

        /**
         */
        virtual void addRef() = 0;

        /**
         */
        virtual void releaseRef() = 0;

        /*!
            Return weak reference
        */
        virtual ::my::IWeakRef* getWeakRef() = 0;

        /**
         */
        virtual uint32_t getRefsCount() const = 0;
    };

    /*
     */
    struct MY_ABSTRACT_TYPE IWeakRef
    {
        virtual ~IWeakRef() = default;

        virtual void addWeakRef() = 0;

        virtual void releaseRef() = 0;

        virtual ::my::IRefCounted* acquire() = 0;

        virtual bool isDead() const = 0;
    };

}  // namespace my
