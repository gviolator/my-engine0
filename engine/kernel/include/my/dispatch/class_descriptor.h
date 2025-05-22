// #my_engine_source_file
#pragma once

#include <optional>

#include "my/dispatch/dispatch_args.h"
#include "my/meta/runtime_attribute.h"
#include "my/rtti/ptr.h"
#include "my/rtti/rtti_object.h"
#include "my/serialization/runtime_value.h"
#include "my/utils/result.h"
#include "my/utils/string_utils.h"
#include "my/utils/uni_ptr.h"

namespace my
{
    /**
     */
    enum class MethodCategory
    {
        Instance,
        Class
    };

    /**
        @brief Dynamic method description
     */
    struct MY_ABSTRACT_TYPE IMethodInfo
    {
        virtual ~IMethodInfo() = default;

        virtual std::string getName() const = 0;

        virtual MethodCategory getCategory() const = 0;

        virtual std::optional<unsigned> getParametersCount() const = 0;

        /**
            @brief Dynamically (without type knowing) invoking the method.
                  The caller must take care to properly destroy the returned object according to its type !

            @return The invocation result. Currently can represent:
                - any value through RuntimeValue api
                - invocable object (i.e. functor/lambda) through IDispatch interface
                - any other IRttiOject based object. Be aware
         */
        virtual Result<UniPtr<IRttiObject>> invoke(IRttiObject* instance, DispatchArguments args) const = 0;

        // /**
        //     @brief temporary method.
        //         MUST be removed after invoke will be refactored to returns Universal ptr
        //  */
        // nau::Ptr<> invokeToPtr(IRttiObject* instance, DispatchArguments args) const
        // {
        //     Result<IRttiObject*> result = this->invoke(instance, std::move(args));
        //     IRttiObject* const obj = *result;
        //     if (!obj)
        //     {
        //         return {};
        //     }

        //     IRefCounted* const refCounted = obj->as<IRefCounted*>();
        //     NAU_FATAL(refCounted);

        //     return rtti::TakeOwnership{refCounted};
        // }
    };

    /**
        @brief Dynamic interface/API description
     */
    struct MY_ABSTRACT_TYPE IInterfaceInfo
    {
        virtual std::string getName() const = 0;

        virtual rtti::TypeInfo getTypeInfo() const = 0;

        virtual size_t getMethodsCount() const = 0;

        virtual const IMethodInfo& getMethod(size_t) const = 0;
    };

    /**
        @brief Dynamic type description
     */
    struct MY_ABSTRACT_TYPE IClassDescriptor : virtual IRefCounted
    {
        MY_INTERFACE(my::IClassDescriptor, IRefCounted)

        // using Ptr = nau::Ptr<IClassDescriptor>;

        virtual rtti::TypeInfo getClassTypeInfo() const = 0;

        virtual std::string getClassName() const = 0;

        virtual const meta::IRuntimeAttributeContainer* getClassAttributes() const = 0;

        virtual size_t getInterfaceCount() const = 0;

        virtual const IInterfaceInfo& getInterface(size_t) const = 0;

        /**
            @brief Class's instance construction factory

            @return Constructor method if instance construction is available, nullptr otherwise
         */
        virtual const IMethodInfo* getConstructor() const = 0;

        const IInterfaceInfo* findInterface(rtti::TypeInfo) const;

        template <rtti::WithTypeInfo>
        const IInterfaceInfo* findInterface() const;

        bool hasInterface(rtti::TypeInfo) const;

        template <rtti::WithTypeInfo>
        bool hasInterface() const;

        const IMethodInfo* findMethod(std::string_view) const;
    };

    using ClassDescriptorPtr = Ptr<IClassDescriptor>;

    /**
     */
    inline const IInterfaceInfo* IClassDescriptor::findInterface(rtti::TypeInfo typeInfo) const
    {
        for (size_t i = 0, iCount = getInterfaceCount(); i < iCount; ++i)
        {
            const IInterfaceInfo& api = getInterface(i);
            const rtti::TypeInfo apiType = api.getTypeInfo();
            if (apiType && (apiType == typeInfo))
            {
                return &api;
            }
        }

        return nullptr;
    }

    template <rtti::WithTypeInfo T>
    inline const IInterfaceInfo* IClassDescriptor::findInterface() const
    {
        return findInterface(rtti::getTypeInfo<T>());
    }

    inline bool IClassDescriptor::hasInterface(rtti::TypeInfo type) const
    {
        return findInterface(type) != nullptr;
    }

    template <rtti::WithTypeInfo T>
    inline bool IClassDescriptor::hasInterface() const
    {
        return findInterface(rtti::getTypeInfo<T>()) != nullptr;
    }

    /**
     */
    inline const IMethodInfo* IClassDescriptor::findMethod(std::string_view methodName) const
    {
        for (size_t i = 0, iCount = getInterfaceCount(); i < iCount; ++i)
        {
            const IInterfaceInfo& api = getInterface(i);
            for (size_t j = 0, mCount = api.getMethodsCount(); j < mCount; ++j)
            {
                const auto& method = api.getMethod(j);
                if (strings::icaseEqual(methodName, method.getName()))
                {
                    return &method;
                }
            }
        }

        return nullptr;
    }
}  // namespace my
