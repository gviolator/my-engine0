// #my_engine_source_file
#include "my/serialization/native_runtime_value/native_object.h"

namespace my::ser_detail
{

    std::string_view RuntimeObjectState::getKey(size_t index) const
    {
        auto fields = getFields();

        MY_DEBUG_ASSERT(index < fields.size());
        return fields[index].getName();
    }

    RuntimeValuePtr RuntimeObjectState::getValue(const RuntimeValue& parent, const void* obj, std::string_view key) const
    {
        auto fields = getFields();

        auto field = std::find_if(fields.begin(), fields.end(), [key](const RuntimeFieldAccessor& field)
        {
            return strings::icaseEqual(field.getName(), key);
        });

        return field != fields.end() ? field->getRuntimeValue(parent, const_cast<void*>(obj)) : nullptr;
    }

    bool RuntimeObjectState::containsKey(std::string_view key) const
    {
        return findField(key) != nullptr;
    }

    Result<> RuntimeObjectState::setFieldValue(const RuntimeValue& parent, const void* obj, std::string_view key, const RuntimeValuePtr& value)
    {
        auto* const field = findField(key);
        if (field == nullptr)
        {
            return MakeError("Class does not contains field:({})", key);
        }

        return RuntimeValue::assign(field->getRuntimeValue(parent, const_cast<void*>(obj)), value);
    }

    RuntimeFieldAccessor* RuntimeObjectState::findField(std::string_view key) const
    {
        const std::span<RuntimeFieldAccessor> fields = getFields();

        auto field = std::find_if(fields.begin(), fields.end(), [key](const RuntimeFieldAccessor& field)
        {
            return strings::icaseEqual(field.getName(), key);
        });

        return field != fields.end() ? &(*field) : nullptr;
    }
}  // namespace my::ser_detail
