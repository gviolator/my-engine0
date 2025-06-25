// #my_engine_source_file
#pragma once

#include <memory>
#include <span>

#include "my/kernel/kernel_config.h"
#include "my/rtti/ptr.h"
#include "my/rtti/rtti_object.h"
#include "my/rtti/weak_ptr.h"

namespace my
{
    /**
     */
    class MY_ABSTRACT_TYPE RuntimeObjectRegistry
    {
    public:
        virtual ~RuntimeObjectRegistry() = default;

        template <typename Callback>
        requires(std::is_invocable_v<Callback, std::span<IRttiObject*>>)
        void visitAllObjects(Callback callback)
        {
            const auto callbackHelper = [](std::span<IRttiObject*> objects, void* callbackData)
            {
                (*reinterpret_cast<Callback*>(callbackData))(objects);
            };

            visitObjects(callbackHelper, rtti::TypeInfo{}, &callback);
        }

        template <typename T, typename Callback>
        requires(std::is_invocable_v<Callback, std::span<IRttiObject*>>)
        void visitObjects(Callback callback)
        {
            const auto callbackHelper = [](std::span<IRttiObject*> objects, void* callbackData)
            {
                (*reinterpret_cast<Callback*>(callbackData))(objects);
            };

            visitObjects(callbackHelper, rtti::getTypeInfo<T>(), &callback);
        }

    protected:
        using ObjectId = uint64_t;
        using VisitObjectsCallback = void (*)(std::span<IRttiObject*>, void*);

        virtual void visitObjects(VisitObjectsCallback callback, rtti::TypeInfo, void*) = 0;

        friend class RuntimeObjectRegistration;
    };

    /**
     */
    class [[nodiscard]] MY_KERNEL_EXPORT RuntimeObjectRegistration
    {
    public:
        RuntimeObjectRegistration();
        RuntimeObjectRegistration(my::Ptr<>);
        RuntimeObjectRegistration(IRttiObject&);
        RuntimeObjectRegistration(RuntimeObjectRegistration&&);
        RuntimeObjectRegistration(const RuntimeObjectRegistration&) = delete;
        ~RuntimeObjectRegistration();

        RuntimeObjectRegistration& operator=(RuntimeObjectRegistration&&);
        RuntimeObjectRegistration& operator=(const RuntimeObjectRegistration&) = delete;
        RuntimeObjectRegistration& operator=(std::nullptr_t);

        explicit operator bool() const;

        /**
            @brief Allow to not keep RuntimeObjectRegistration.

            Registered object MUST provide weak reference mechanism (currently only for IRefCounted).
        */
        void setAutoRemove();

    private:
        void reset();

        RuntimeObjectRegistry::ObjectId m_objectId;

        friend class RuntimeObjectRegistryImpl;
    };

    MY_KERNEL_EXPORT RuntimeObjectRegistry& getRuntimeObjectRegistry();

    MY_KERNEL_EXPORT bool hasRuntimeObjectRegistry();

    MY_KERNEL_EXPORT void setDefaultRuntimeObjectRegistryInstance();

    MY_KERNEL_EXPORT void resetRuntimeObjectRegistryInstance();
}  // namespace my
