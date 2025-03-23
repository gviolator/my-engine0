// #my_engine_source_header

#include "my/rtti/rtti_impl.h"
#include "my/rtti/type_info.h"

namespace my::test
{
    struct MyTypeWithTypeId
    {
        MY_TYPEID(MyTypeWithTypeId);
    };

    struct MyTypeWithTypeId2
    {
    };

    struct MyTypeNoTypeId
    {
    };

}  // namespace my::test

MY_DECLARE_TYPEID(my::test::MyTypeWithTypeId2)

namespace my::test
{
    TEST(TestTypeInfo, HasTypeInfo)
    {
        static_assert(rtti::HasTypeInfo<MyTypeWithTypeId>);
        static_assert(rtti::HasTypeInfo<MyTypeWithTypeId2>);
        static_assert(!rtti::HasTypeInfo<MyTypeNoTypeId>);
    }

    TEST(TestTypeInfo, GetTypeInfo)
    {
        ASSERT_NE(rtti::getTypeInfo<MyTypeWithTypeId>().getHashCode(), 0);
        ASSERT_NE(rtti::getTypeInfo<MyTypeWithTypeId2>().getHashCode(), 0);
        ASSERT_NE(rtti::getTypeInfo<MyTypeWithTypeId2>().getHashCode(), rtti::getTypeInfo<MyTypeWithTypeId>().getHashCode());
    }

    TEST(TestTypeInfo, FromId)
    {
        const rtti::TypeInfo& typeInfo = rtti::getTypeInfo<MyTypeWithTypeId>();
        const auto tId = typeInfo.getHashCode();

        rtti::TypeInfo typeInfo2 = rtti::TypeInfo::fromId(typeInfo.getHashCode());
        ASSERT_TRUE(typeInfo2);
        ASSERT_EQ(typeInfo2, typeInfo);
    }

    TEST(TestTypeInfo, FromTypeName)
    {
        const rtti::TypeInfo& typeInfo = rtti::getTypeInfo<MyTypeWithTypeId>();
        const auto typeName = typeInfo.getTypeName();

        rtti::TypeInfo typeInfo2 = rtti::TypeInfo::fromName(std::string{typeName}.c_str());
        ASSERT_TRUE(typeInfo2);
        ASSERT_EQ(typeInfo2, typeInfo);
    }

    TEST(TestTypeInfo, Comparison)
    {
        const auto& typeInfo1 = rtti::getTypeInfo<MyTypeWithTypeId>();
        const auto& typeInfo2 = rtti::getTypeInfo<MyTypeWithTypeId2>();

        ASSERT_EQ(typeInfo1, rtti::getTypeInfo<MyTypeWithTypeId>());
        ASSERT_EQ(typeInfo2, rtti::getTypeInfo<MyTypeWithTypeId2>());

        ASSERT_NE(typeInfo2, typeInfo1);
    }

    TEST(TestTypeInfo, AsKey)
    {
        using namespace my::rtti;

        const std::map<TypeInfo, std::string> typeNames = {
            { getTypeInfo<MyTypeWithTypeId>(), "one"},
            {getTypeInfo<MyTypeWithTypeId2>(), "two"}
        };

        ASSERT_EQ(std::string{"one"}, typeNames.at(getTypeInfo<MyTypeWithTypeId>()));
        ASSERT_EQ(std::string{"two"}, typeNames.at(getTypeInfo<MyTypeWithTypeId2>()));
    }
}  // namespace my::test
