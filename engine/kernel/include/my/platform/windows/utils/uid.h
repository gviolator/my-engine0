// #my_engine_source_header


#pragma once

#include <EASTL/functional.h>
#include <guiddef.h>

#include <string>
#include <string_view>
#include <type_traits>

#include "my/kernel/kernel_config.h"
#include "my/rtti/type_info.h"
#include "my/utils/result.h"

namespace my
{

    /**

    */
    struct MY_KERNEL_EXPORT Uid
    {
        static Uid generate();
        static Result<Uid> parseString(std::string_view);

        /**
         */
        Uid() noexcept;
        Uid(const Uid&) = default;
        Uid& operator=(const Uid&) = default;

        /**
         */
        explicit operator bool() const noexcept;

    private:
        Uid(GUID data) noexcept;

        size_t getHashCode() const;

        GUID m_data;

        /**
         */
        MY_KERNEL_EXPORT friend Result<> parse(std::string_view str, Uid&);
        MY_KERNEL_EXPORT friend std::string toString(const Uid& uid);
        MY_KERNEL_EXPORT friend bool operator<(const Uid& uid, const Uid& uidOther) noexcept;
        MY_KERNEL_EXPORT friend bool operator==(const Uid& uid, const Uid& uidOther) noexcept;
        MY_KERNEL_EXPORT friend bool operator!=(const Uid& uid, const Uid& uidOther) noexcept;

        friend std::hash<my::Uid>;
        friend std::hash<my::Uid>;
    };

    MY_KERNEL_EXPORT std::string toString(const Uid& uid);

}  // namespace my

MY_DECLARE_TYPEID(my::Uid)

template <>
struct std::hash<my::Uid>
{
    [[nodiscard]] size_t operator()(const my::Uid& val) const
    {
        return val.getHashCode();
    }
};

template <>
struct std::hash<my::Uid>
{
    [[nodiscard]] size_t operator()(const my::Uid& val) const
    {
        return val.getHashCode();
    }
};
