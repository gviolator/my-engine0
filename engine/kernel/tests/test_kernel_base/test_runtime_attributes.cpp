// #my_engine_source_file
#if 0
#include "my/meta/runtime_attribute.h"
#include "my/rtti/rtti_impl.h"
#include "my/rtti/type_info.h"
#include "my/serialization/runtime_value_builder.h"
#include "my/utils/tuple_utility.h"
#include "my/utils/type_utility.h"

namespace my::test
{
    namespace
    {

        MY_DEFINE_ATTRIBUTE(Attrib0, "attrib_0", meta::AttributeOptionsNone)
        MY_DEFINE_ATTRIBUTE(Attrib1, "attrib_1", meta::AttributeOptions::Inherited)
        MY_DEFINE_ATTRIBUTE(Attrib2, "attrib_2", meta::AttributeOptionsNone)
        MY_DEFINE_ATTRIBUTE(AttribTag, "attrib_tag", meta::AttributeOptions::Inherited)

        MY_DEFINE_ATTRIBUTE(Attrib_Unnamed, "", meta::AttributeOptionsNone)
        MY_DEFINE_ATTRIBUTE(Attrib_NotRuntime, "not_runtime", meta::AttributeOptionsNone)

        struct NotSerializableStruct
        {
        };

        struct SerializableStruct
        {
            int field1 = 1;
            std::string field2 = "test";

            MY_CLASS_FIELDS(
                CLASS_FIELD(field1),
                CLASS_FIELD(field2))

            [[maybe_unused]] friend bool operator==(const SerializableStruct& s0, const SerializableStruct& s1)
            {
                return s0.field1 == s1.field1 && s0.field2 == s1.field2;
            }
        };

        class MyClass
        {
        public:
            MY_CLASS_ATTRIBUTES(
                CLASS_ATTRIBUTE(Attrib0, std::string{"One"}),
                CLASS_ATTRIBUTE(Attrib1, 11),
                CLASS_ATTRIBUTE(Attrib_Unnamed, 22),
                CLASS_ATTRIBUTE(Attrib_NotRuntime, NotSerializableStruct{}),
                CLASS_ATTRIBUTE(Attrib2, SerializableStruct{}),
                CLASS_ATTRIBUTE(AttribTag, std::string{"tag_0"}))
        };

        class MyClass2 : public MyClass
        {
            MY_CLASS_BASE(MyClass)

            MY_CLASS_ATTRIBUTES(
                CLASS_ATTRIBUTE(AttribTag, std::string{"tag_1"}))
        };
    }  // namespace

    /**
        Test:
            Compile time traits
     */
    TEST(TestRuntimeAttribute, AttributesContainerTraits)
    {
        using namespace my::meta;

        static_assert(!std::is_default_constructible_v<RuntimeAttributeContainer>);
        static_assert(std::is_copy_constructible_v<RuntimeAttributeContainer>);
        static_assert(std::is_move_constructible_v<RuntimeAttributeContainer>);
        static_assert(std::is_copy_assignable_v<RuntimeAttributeContainer>);
        static_assert(std::is_move_assignable_v<RuntimeAttributeContainer>);
    }

    /**
        Test:
            Simple create container
     */
    TEST(TestRuntimeAttribute, MakeAttributesContainer)
    {
        meta::RuntimeAttributeContainer container = meta::makeRuntimeAttributeContainer<MyClass>();
        ASSERT_EQ(container.getSize(), 4);  // Attrib0, Attrib1, Attrib2, AttribTag
    }

    /**
        Test:
            Attributes with non-serializable values ​​and empty string keys
            should not be accessible through the attribute container.
     */
    TEST(TestRuntimeAttribute, ContainsOnlyAllowedAttributes)
    {
        meta::RuntimeAttributeContainer container = meta::makeRuntimeAttributeContainer<MyClass>();
        ASSERT_TRUE(container.containsAttribute(Attrib0{}.strValue));
        ASSERT_TRUE(container.containsAttribute(Attrib1{}.strValue));
        ASSERT_TRUE(container.containsAttribute(Attrib2{}.strValue));
        ASSERT_TRUE(container.containsAttribute(AttribTag{}.strValue));
    }

    /**
        Test:
            Checks attribute actual value
     */
    TEST(TestRuntimeAttribute, AttributeValue)
    {
        meta::RuntimeAttributeContainer container = meta::makeRuntimeAttributeContainer<MyClass>();

        ASSERT_TRUE(container.getValue("attrib_0")->is<StringValue>());
        ASSERT_TRUE(container.getValue("attrib_1")->is<IntegerValue>());
        ASSERT_TRUE(container.getValue("attrib_2")->is<RuntimeObject>());
        ASSERT_TRUE(container.getValue("attrib_tag")->is<StringValue>());

        ASSERT_EQ(*runtimeValueCast<std::string>(container.getValue("attrib_0")), "One");
        ASSERT_EQ(*runtimeValueCast<int>(container.getValue("attrib_1")), 11);
        ASSERT_EQ(*runtimeValueCast<std::string>(container.getValue("attrib_tag")), "tag_0");
        ASSERT_EQ(*runtimeValueCast<SerializableStruct>(container.getValue("attrib_2")), SerializableStruct{});
    }

    TEST(TestRuntimeAttribute, AttributeValue_2)
    {
        meta::RuntimeAttributeContainer container = meta::makeRuntimeAttributeContainer<MyClass>();

        const auto value_0 = container.get<Attrib0, std::string>().value_or("");
        const auto value_1 = container.get<Attrib1, int>().value_or(0);
        const auto value_2 = container.get<Attrib2, SerializableStruct>();
        const auto value_tag = container.get<AttribTag, std::string>().value_or("");
        
        ASSERT_TRUE(value_2);

        ASSERT_EQ(value_0, "One");
        ASSERT_EQ(value_1, 11);
        ASSERT_EQ(value_tag, "tag_0");
        ASSERT_EQ(value_2, SerializableStruct{});
    }

    /**
        Test:
            Checks attribute inheritance (only attributes with meta::AttributeOptions::Inherited option are inherited)
     */
    TEST(TestRuntimeAttribute, AttributeInheritance)
    {
        meta::RuntimeAttributeContainer container = meta::makeRuntimeAttributeContainer<MyClass2>();
        ASSERT_FALSE(container.contains<Attrib0>());   // not inherited
        ASSERT_TRUE(container.contains<Attrib1>());    // inherited
        ASSERT_FALSE(container.contains<Attrib2>());   // not inherited
        ASSERT_TRUE(container.contains<AttribTag>());  // inherited
    }

    /**
        Test:
            Multiple values for the same attribute
            TODO: there is must be ability to override (not append) inherited attributes (currently not implemented)
     */
    TEST(TestRuntimeAttribute, MultipleAttributeValue)
    {
        meta::RuntimeAttributeContainer container = meta::makeRuntimeAttributeContainer<MyClass2>();
        ASSERT_EQ(container.getSize(), 2);
        ASSERT_EQ(container.getAll<Attrib1>().size(), 1);

        auto tagValues = container.getAll<AttribTag>();
        ASSERT_EQ(tagValues.size(), 2);

        std::vector<std::string> tags = {
            *runtimeValueCast<std::string>(tagValues[0]),
            *runtimeValueCast<std::string>(tagValues[1])};
        std::sort(tags.begin(), tags.end());

        ASSERT_EQ(tags[0], "tag_0");
        ASSERT_EQ(tags[1], "tag_1");
    }

}  // namespace my::test
#endif