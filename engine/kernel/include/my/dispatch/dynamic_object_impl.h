#if 0
// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include "my/dispatch/class_descriptor_builder.h"
#include "my/dispatch/dynamic_object.h"
#include "my/kernel/kernel_config.h"
#include "my/serialization/runtime_value_builder.h"

namespace nau
{
    /**
     */
    class MY_ABSTRACT_TYPE DynamicObjectImpl : public DynamicObject,
                                                public ser_detail::NativeRuntimeValueBase<RuntimeObject>
    {
        using RuntimeValueBase = ser_detail::NativeRuntimeValueBase<RuntimeObject>;

        MY_INTERFACE(nau::DynamicObjectImpl, DynamicObject, RuntimeValueBase)

    public:
        bool isMutable() const override
        {
            return true;
        }

        size_t getSize() const override
        {
            return getRuntimeObjectState().getSize();
        }

        std::string_view getKey(size_t index) const override
        {
            return getRuntimeObjectState().getKey(index);
        }

        RuntimeValuePtr getValue(std::string_view key) override
        {
            auto fieldValue = getRuntimeObjectState().getValue(static_cast<const RuntimeValue&>(*this), getThis(), key);
            NAU_FATAL(fieldValue);

            return fieldValue;
        }

        bool containsKey(std::string_view key) const override
        {
            return getRuntimeObjectState().containsKey(key);
        }

        Result<> setValue(std::string_view key, const RuntimeValuePtr& value) override
        {
            // there is no need to tracking changes, because changes notification will propagate from
            // child field (that will be created inside setFieldValue).
            return getRuntimeObjectState().setFieldValue(*this, getThis(), key, value);
        }

        std::optional<FieldInfo> findFieldInfo(std::string_view) const override
        {
            return std::nullopt;
        }

    protected:
        virtual eastl::unique_ptr<ser_detail::RuntimeObjectState> createRuntimeObjectState() const = 0;

        virtual void* getThis() const = 0;

    private:
        ser_detail::RuntimeObjectState& getRuntimeObjectState() const
        {
            if(!m_runtimeObjectState)
            {
                m_runtimeObjectState = createRuntimeObjectState();
                NAU_FATAL(m_runtimeObjectState);
            }

            return *m_runtimeObjectState;
        }

        mutable eastl::unique_ptr<ser_detail::RuntimeObjectState> m_runtimeObjectState;
    };

}  // namespace nau

#define NAU_DECLARE_DYNAMIC_OBJECT                                                                      \
    ::nau::IClassDescriptor::Ptr getClassDescriptor() const override;                                   \
    eastl::unique_ptr<::nau::ser_detail::RuntimeObjectState> createRuntimeObjectState() const override; \
    void* getThis() const override;

#define NAU_IMPLEMENT_DYNAMIC_OBJECT(Class)                                                          \
    ::nau::IClassDescriptor::Ptr Class::getClassDescriptor() const                                   \
    {                                                                                                \
        return ::nau::getClassDescriptor<Class>();                                                   \
    }                                                                                                \
                                                                                                     \
    eastl::unique_ptr<::nau::ser_detail::RuntimeObjectState> Class::createRuntimeObjectState() const \
    {                                                                                                \
        return eastl::make_unique<::nau::ser_detail::RuntimeObjectStateImpl<Class>>();               \
    }                                                                                                \
                                                                                                     \
    void* Class::getThis() const                                                                     \
    {                                                                                                \
        using ThisType = std::remove_const_t<std::remove_reference_t<decltype(*this)>>;              \
        auto mutableThis = const_cast<ThisType*>(this);                                              \
        return reinterpret_cast<void*>(mutableThis);                                                 \
    }

#endif    