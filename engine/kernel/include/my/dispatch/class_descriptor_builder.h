// #my_engine_source_file
#pragma once

#include "my/dispatch/class_descriptor.h"
#include "my/dispatch/dispatch.h"
#include "my/meta/class_info.h"
#include "my/meta/common_attributes.h"
#include "my/meta/function_info.h"
#include "my/rtti/rtti_impl.h"
#include "my/rtti/weak_ptr.h"
#include "my/serialization/runtime_value_builder.h"
#include "my/threading/spin_lock.h"
#include "my/utils/functor.h"
#include "my/utils/tuple_utility.h"
#include "my/utils/type_list/append.h"

namespace my::kernel_detail
{

    template <typename T>
    Result<> assignNativeValue(const my::Ptr<>& inArg, T& value);

    template <typename... T>
    Result<> assignArgumentValues(const DispatchArguments& inArgs, T&... outValues);

    template <typename T>
    requires(HasRuntimeValueRepresentation<T> || meta::IsCallable<T>)
    my::Ptr<> makeRuntimeValue(T&& value);

    // SFINAE helper to detect that type/class has custom instance constructor method: Type::classCreateInstance<T>(A...);
    template <typename T, typename... A>
    decltype(T::template classCreateInstance<T>(constLValueRef<A>...), std::true_type{}) hasClassCreateInstanceHelper(int);

    template <typename T, typename... A>
    std::false_type hasClassCreateInstanceHelper(...);

    template <typename T, typename... A>
    constexpr bool HasClassCreateInstance = decltype(hasClassCreateInstanceHelper<T, A...>(int{}))::value;

    template <typename T>
    static inline constexpr bool ClassHasConstructorMethod =
        std::is_base_of_v<IRefCounted, T> || HasClassCreateInstance<T> || std::is_constructible_v<T>;

    template <typename T>
    std::string getTypeName()
    {
        std::string name;

        if constexpr (meta::ClassHasName<T>)
        {
            name = meta::get_class_name<T>();
        }
        else
        {
            if (std::string_view typeName = rtti::getTypeInfo<T>().getTypeName(); !typeName.empty())
            {
                name.assign(typeName.data(), typeName.size());
            }
            else
            {
                name = "Unnamed_Class";
            }
        }
        // TODO: modify class name if required (for example replace "::" with "_")
        return name;
    }

    /**
     */
    template <typename F, bool Const, bool NoExcept, typename Class, typename Res, typename... P>
    auto makeFunctionalDispatchWrapper(my::Ptr<> dispatchPtr, meta::CallableTypeInfo<Const, NoExcept, Class, Res, P...>)
    {
        NAU_ASSERT(dispatchPtr);
        NAU_ASSERT(dispatchPtr->is<IDispatch>());

        return [dispatchPtr = std::move(dispatchPtr)](P... p) -> Res
        {
            constexpr size_t ArgsCount = sizeof...(P);

            DispatchArguments dispatchArgs;
            if constexpr (ArgsCount > 0)
            {
                dispatchArgs.reserve(ArgsCount);
                (dispatchArgs.emplace_back(makeRuntimeValue(std::move(p))), ...);
            }

            auto& dispatch = dispatchPtr->as<IDispatch&>();
            Result<my::Ptr<>> invokeResult = dispatch.invoke({}, {}, std::move(dispatchArgs));

            return {};
        };
    }

    template <typename F, typename = meta::GetCallableTypeInfo<F>>
    class FunctionDispatchImpl;

    template <typename F, bool Const, bool NoExcept, typename Class, typename Res, typename... P>
    class FunctionDispatchImpl<F, meta::CallableTypeInfo<Const, NoExcept, Class, Res, P...>> final : public IDispatch,
                                                                                                     public virtual IRefCounted
    {
        MY_REFCOUNTED_CLASS(FunctionDispatchImpl, IDispatch);

    public:
        FunctionDispatchImpl(F callable) :
            m_callable(std::move(callable))
        {
        }

        Result<my::Ptr<>> invoke([[maybe_unused]] std::string_view contract, [[maybe_unused]] std::string_view method, DispatchArguments args) override
        {
            NAU_ASSERT(contract.empty());
            NAU_ASSERT(method.empty());
            NAU_ASSERT("Not implemented");
            return nullptr;
        }

        ClassDescriptorPtr getClassDescriptor() const override
        {
            return nullptr;
        }

    private:
        F m_callable;
    };

    template <typename T>
    Result<> assignNativeValue(const my::Ptr<>& inArg, T& outValue)
    {
        if constexpr (HasRuntimeValueRepresentation<T>)
        {
            if (auto* const rtArgValue = inArg->as<RuntimeValue*>())
            {
                return RuntimeValue::assign(makeValueRef(outValue), my::Ptr{rtArgValue});
            }
            else
            {
                return MakeError("Dont known how to assign value");
            }
        }
        else if constexpr (meta::IsCallable<T>)
        {
            auto* dispatch = inArg->as<IDispatch*>();
            NAU_ASSERT(dispatch, "Expected IDispatch api");

            outValue = makeFunctionalDispatchWrapper<T>(inArg, meta::GetCallableTypeInfo<T>{});
            return {};
        }

        return MakeError("Dont known how to assign arg value");
    }

    template <typename... A>
    Result<> assignArgumentValues(const DispatchArguments& inArgs, A&... outValues)
    {
        static_assert(sizeof...(A) == 0 || !(std::is_reference_v<A> || ...));

        Error::Ptr error;
        size_t argIndex = 0;

        if (!(assignNativeValue(inArgs[argIndex++], outValues).isSuccess(&error) && ...))
        {
            return error;
        }

        return {};
    }

    template <typename T>
    requires(HasRuntimeValueRepresentation<T> || meta::IsCallable<T>)
    my::Ptr<> makeRuntimeValue(T&& value)
    {
        if constexpr (HasRuntimeValueRepresentation<T>)
        {
            return makeValueCopy(std::forward<T>(value));
        }
        else
        {
            static_assert(meta::IsCallable<T>);
            using FunctionDispatch = FunctionDispatchImpl<std::decay_t<T>>;

            return rtti::createInstance<FunctionDispatch, IRefCounted>(std::move(value));
        }
    }

    template <typename T>
    class MethodInfoImpl;

    template <auto F, typename... A>
    class MethodInfoImpl<meta::MethodInfo<F, A...>> : public IMethodInfo
    {
        using MethodInfo = meta::MethodInfo<F, A...>;

    public:
        MethodInfoImpl(MethodInfo methodInfo) :
            m_methodInfo(methodInfo)
        {
        }

        std::string getName() const override
        {
            return std::string{m_methodInfo.getName()};
        }

        MethodCategory getCategory() const override
        {
            if constexpr (MethodInfo::IsMemberFunction)
            {
                return MethodCategory::Instance;
            }
            else
            {
                return MethodCategory::Class;
            }
        }

        std::optional<unsigned> getParametersCount() const override
        {
            using Params = typename MethodInfo::FunctionTypeInfo::ParametersList;
            return Params::Size;
        }

        Result<UniPtr<IRttiObject>> invoke(IRttiObject* instance, DispatchArguments args) const override
        {
            return invokeImpl(instance, typename MethodInfo::FunctionTypeInfo{}, args);
        }

    private:
        template <bool Const, bool NoExcept, typename C, typename R, typename... P>
        static Result<UniPtr<IRttiObject>> invokeImpl(IRttiObject* instance, meta::CallableTypeInfo<Const, NoExcept, C, R, P...> callableInfo, DispatchArguments& inArgs)
        {
            if constexpr (MethodInfo::IsMemberFunction)
            {
                return invokeInstanceImpl<C, R>(instance, inArgs, std::remove_const_t<std::remove_reference_t<P>>{}...);
            }
            else
            {  // INVOKE STATIC
                return nullptr;
            }
        }

        template <typename Class, typename R, typename... P>
        static Result<UniPtr<IRttiObject>> invokeInstanceImpl(IRttiObject* instance, DispatchArguments& inArgs, P... arguments)
        {
            MY_DEBUG_CHECK(instance);
            CheckResult(assignArgumentValues(inArgs, arguments...));

            Class* const api = instance->as<Class*>();
            MY_DEBUG_CHECK(api);
            if (!api)
            {
                return MakeError("Api not supported");
            }

            if constexpr (std::is_same_v<R, void>)
            {
                (api->*F)(std::move(arguments)...);
                return nullptr;
            }
            else
            {
                auto result = (api->*F)(std::move(arguments)...);
                if constexpr (HasRuntimeValueRepresentation<decltype(result)>)
                {
                    RuntimeValuePtr resultAsRuntimeValue;

                    if constexpr (std::is_lvalue_reference_v<decltype(result)>)
                    {
                        resultAsRuntimeValue = makeValueRef(result);
                    }
                    else
                    {
                        resultAsRuntimeValue = makeValueCopy(result);
                    }

                    MY_FATAL(resultAsRuntimeValue);

                    // RuntimeValue* const runtimeResult = resultAsRuntimeValue.giveUp();
                    return UniPtr<IRttiObject>{resultAsRuntimeValue};
                }
                else
                {
                    // TODO supports for:
                    // - invocable objects (functor/lambda) as IDispatch interface
                    // - any IRttiObject base objects

                    MY_FATAL("Returning non runtime values is not implemented");
                }
            }

            return nullptr;
        }

        const MethodInfo m_methodInfo;
    };

    template <typename>
    struct GetMethodInfoImplTuple;

    template <typename... T>
    struct GetMethodInfoImplTuple<std::tuple<T...>>
    {
        using type = std::tuple<MethodInfoImpl<T>...>;
    };

    template <typename T>
    class InterfaceInfoImpl final : public IInterfaceInfo
    {
        using MethodInfoTuple = std::remove_const_t<std::remove_reference_t<decltype(meta::getClassDirectMethods<T>())>>;
        using MethodInfoImplTuple = typename GetMethodInfoImplTuple<MethodInfoTuple>::type;

        template <typename... M>
        inline static MethodInfoImplTuple makeMethodInfos(const std::tuple<M...>& methodInfos)
        {
            return [&]<size_t... I>(std::index_sequence<I...>)
            {
                return std::tuple{std::get<I>(methodInfos)...};
            }(std::make_index_sequence<sizeof...(M)>{});
        }

    public:
        using InterfaceType = T;

        InterfaceInfoImpl() :
            m_methods(makeMethodInfos(meta::getClassDirectMethods<T>()))
        {
        }

        std::string getName() const override
        {
            std::string name = kernel_detail::getTypeName<InterfaceType>();
            return std::string{name.data(), name.size()};
        }

        rtti::TypeInfo getTypeInfo() const override
        {
            if constexpr (rtti::HasTypeInfo<T>)
            {
                return &rtti::getTypeInfo<T>();
            }
            else
            {
                return rtti::TypeInfo{};
            }
        }

        constexpr size_t getMethodsCount() const override
        {
            return std::tuple_size_v<MethodInfoImplTuple>;
        }

        const IMethodInfo& getMethod(size_t i) const override
        {
            NAU_ASSERT(getMethodsCount() > 0 && i < getMethodsCount());

            const IMethodInfo* methodInfoPtr = nullptr;

            if constexpr (std::tuple_size_v<MethodInfoImplTuple> > 0)
            {
                TupleUtils::invokeAt(m_methods, i, [&methodInfoPtr](const IMethodInfo& methodInfo)
                {
                    methodInfoPtr = &methodInfo;
                });
            }

            NAU_ASSERT(methodInfoPtr);

            return *methodInfoPtr;
        }

    private:
        const MethodInfoImplTuple m_methods;
    };

    /**
        @brief Instance constructor implementation
     */
    template <typename T>
    class ClassConstructorImpl final : public IMethodInfo
    {
    public:
        std::string getName() const override
        {
            return ".ctor";
        }

        MethodCategory getCategory() const override
        {
            return MethodCategory::Class;
        }

        std::optional<unsigned> getParametersCount() const override
        {
            return 0;
        }

        Result<UniPtr<IRttiObject>> invoke(IRttiObject*, [[maybe_unused]] DispatchArguments args) const override
        {
            // Prefer custom constructor over default implementation
            // even if instance can be constructed with rtti::createInstanceXXX.
            if constexpr (HasClassCreateInstance<T>)
            {
                return invokeClassCreateInstance(args);
            }
            else if constexpr (std::is_base_of_v<IRefCounted, T>)
            {
                // TODO: need to check first argument is IMemAllocator
                // and using rtti::createInstanceWithAllocator in that case.
                static_assert(std::is_constructible_v<T>, "Type is not default constructible (or not constructible at all)");
                my::Ptr<T> instance = rtti::createInstance<T, IRefCounted>();

                return instance.giveUp()->template as<IRttiObject*>();
            }
            else if constexpr (std::is_constructible_v<T>)
            {
                T* const instance = new T();
                return rtti::staticCast<IRttiObject*>(instance);
            }
            else
            {
                MY_FAILURE("Dynamic Construction currently for this Type is not supported/implemented");
                return MakeError("Dynamic Construction is not implemented");
            }
        }

    private:
        static UniPtr<IRttiObject> invokeClassCreateInstance(const DispatchArguments& args)
        {
            // TODO: classCreateInstance can support any arguments,
            // but currently checking only for allocator
            if (!args.empty())
            {
                MY_DEBUG_CHECK(!args.front() || args.front()->is<IMemAllocator>(), "Instance creation method supports only IMemAllocator as argument");
            }

            Ptr<IRttiObject> resultInstance;
            // if constexpr (std::is_invocable_v<typename T::template classCreateInstance<T>, IMemAllocator*>)
            {
                IMemAllocator* const allocator = nullptr;

                resultInstance = T::template classCreateInstance<T>(allocator);
            }
            // else
            // {
            //     resultInstance = typename T::template classCreateInstance<T>();
            // }

            return resultInstance;
        }
    };

    class ClassConstructorStubImpl final : public IMethodInfo
    {
    public:
        std::string getName() const override
        {
            return ".ctor";
        }

        MethodCategory getCategory() const override
        {
            return MethodCategory::Class;
        }

        std::optional<unsigned> getParametersCount() const override
        {
            return 0;
        }

        Result<UniPtr<IRttiObject>> invoke([[maybe_unused]] IRttiObject*, [[maybe_unused]] DispatchArguments args) const override
        {
            MY_FAILURE("This method must never be called");
            return MakeError("Construction for this type is not supported");
        }
    };

    template <typename T>
    class ClassDescriptorImpl final : public IClassDescriptor
    {
        MY_REFCOUNTED_CLASS(ClassDescriptorImpl<T>, IClassDescriptor)

        using ApiCollection = type_list::Append<meta::ClassAllUniqueBase<T>, T>;
        using InterfaceInfoTuple = my::TupleUtils::TupleFrom<type_list::Transform<ApiCollection, InterfaceInfoImpl>>;

    public:
        ClassDescriptorImpl()
        {
            using namespace my::meta;
            m_attributes = std::make_unique<RuntimeAttributeContainer>(makeRuntimeAttributeContainer<T>());
        }

        rtti::TypeInfo getClassTypeInfo() const override
        {
            return rtti::getTypeInfo<T>();
        }

        std::string getClassName() const override
        {
            std::string name = kernel_detail::getTypeName<T>();
            return std::string{name.data(), name.size()};
        }

        const meta::IRuntimeAttributeContainer* getClassAttributes() const override
        {
            return m_attributes.get();
        }

        size_t getInterfaceCount() const override
        {
            return ApiCollection::Size;
        }

        const IInterfaceInfo& getInterface(size_t i) const override
        {
            NAU_ASSERT(getInterfaceCount() > 0 && i < getInterfaceCount());

            const IInterfaceInfo* interfaceInfoPtr = nullptr;

            TupleUtils::invokeAt(m_interfaces, i, [&interfaceInfoPtr](const IInterfaceInfo& interfaceInfo)
            {
                interfaceInfoPtr = &interfaceInfo;
            });

            NAU_ASSERT(interfaceInfoPtr);

            return *interfaceInfoPtr;
        }

        const IMethodInfo* getConstructor() const override
        {
            if constexpr (ClassHasConstructorMethod<T>)
            {
                return &m_ctorMethod;
            }
            else
            {
                return nullptr;
            }
        }

    private:
        using ClassConstructorMethod = std::conditional_t<ClassHasConstructorMethod<T>, ClassConstructorImpl<T>, ClassConstructorStubImpl>;

        const InterfaceInfoTuple m_interfaces;
        const ClassConstructorMethod m_ctorMethod;
        std::unique_ptr<meta::RuntimeAttributeContainer> m_attributes;
    };

}  // namespace my::kernel_detail

namespace my
{
    template <typename T>
    ClassDescriptorPtr getClassDescriptor()
    {
        static WeakPtr<IClassDescriptor> descriptorWeakRef;
        static threading::SpinLock mutex;

        const std::lock_guard lock{mutex};
        Ptr<IClassDescriptor> classDescriptor = descriptorWeakRef.lock();
        if (!classDescriptor)
        {
            classDescriptor = rtti::createInstance<kernel_detail::ClassDescriptorImpl<T>>();
            descriptorWeakRef = classDescriptor;
        }

        return classDescriptor;
    }

}  // namespace my
