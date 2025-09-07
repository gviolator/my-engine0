// #my_engine_source_file

#pragma once

#include "my/app/property_container.h"
#include "my/rtti/rtti_impl.h"
#include "my/utils/string_utils.h"

namespace my
{
    /**
     */
    class PropertyContainerImpl final : public PropertyContainer
    {
        MY_RTTI_CLASS(my::PropertyContainerImpl, PropertyContainer)

    public:
        PropertyContainerImpl();

        bool contains(std::string_view path) const override;

        RuntimeValuePtr getRead(std::string_view path, ReadOnlyLock& lock,  IAllocator* allocator) const override;

        Result<RuntimeValuePtr> getModify(std::string_view path, ModificationLock& lock, IAllocator* allocator) override;

        Result<> set(std::string_view path, RuntimeValuePtr value) override;

        Result<> mergeWithValue(const RuntimeValue& value) override;

        void addVariableResolver(std::string_view kind, VariableResolverCallback resolver) override;

    private:
        RuntimeValuePtr findValueAtPath(std::string_view valuePath) const;

        Result<Ptr<Dictionary>> getDictionaryAtPath(std::string_view valuePath, bool createPath = true);

        std::optional<std::string> expandConfigString(std::string_view str) const;

        Ptr<Dictionary> m_propsRoot;
        std::map<std::string, VariableResolverCallback, strings::CiStringComparer<std::string_view>> m_variableResolvers;
        mutable std::shared_mutex m_mutex;
    };
}  // namespace my
