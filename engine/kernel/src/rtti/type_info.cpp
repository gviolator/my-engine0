// #my_engine_source_file
#include "my/rtti/type_info.h"

#include "my/diag/check.h"
#include "my/threading/lock_guard.h"

template <>
struct std::hash<::my::rtti_detail::TypeId>
{
    [[nodiscard]]
    size_t operator()(const ::my::rtti_detail::TypeId& val) const
    {
        return val.typeId;
    }
};

namespace my::rtti_detail
{
    struct TypeEntry
    {
        std::string_view typeName;
    };

    struct TypesRegistration
    {
        std::shared_mutex mutex;
        std::unordered_map<TypeId, TypeEntry> types;

        TypesRegistration() = default;
        TypesRegistration(const TypesRegistration&) = delete;
        TypesRegistration& operator=(const TypesRegistration&) = delete;
    };

    TypesRegistration& getTypesRegistration()
    {
        static TypesRegistration reg;

        return (reg);
    }

    TypeId registerRuntimeTypeInternal(const size_t typeHash, std::string_view typeName)
    {
        MY_DEBUG_CHECK(typeHash != 0);
        MY_DEBUG_CHECK(!typeName.empty());

        if (typeHash == 0 || typeName.empty())
        {
            return {};
        }

        while (typeName.back() == '\0')
        {
            typeName.remove_suffix(1);
        }

        auto& reg = getTypesRegistration();

        lock_(reg.mutex);

        [[maybe_unused]] auto [iter, emplaceOk] = reg.types.try_emplace(typeHash, TypeEntry{typeName});
        MY_DEBUG_CHECK(emplaceOk || iter->second.typeName == typeName);
        return iter->first;
    }

}  // namespace my::rtti_detail

namespace my::rtti
{

    TypeInfo TypeInfo::fromId(size_t typeId)
    {
        auto& reg = rtti_detail::getTypesRegistration();

        shared_lock_(reg.mutex);

        const auto iter = reg.types.find(rtti_detail::TypeId{typeId});
        return iter != reg.types.end() ? TypeInfo{typeId, iter->second.typeName} : TypeInfo{};
    }

    TypeInfo TypeInfo::fromName(const char* name)
    {
        if (!name)
        {
            return {};
        }

        auto& reg = rtti_detail::getTypesRegistration();

        shared_lock_(reg.mutex);

        for (const auto [typeId, entry] : reg.types)
        {
            if (entry.typeName == name)
            {
                return TypeInfo{typeId, entry.typeName};
            }
        }

        return {};
    }

}  // namespace my::rtti
