// #my_engine_source_file

#include "my/serialization/json.h"
#include "my/serialization/json_utils.h"

namespace my::test
{
    /**
     */
    TEST(TestSerializationJsonMutability, SetPrimitive)
    {
        Ptr<RuntimeDictionary> dict = serialization::jsonCreateDictionary();
        dict->setValue("fieldInt", makeValueCopy(111)).ignore();
        dict->setValue("fieldSingle", makeValueCopy(222.2f)).ignore();
        dict->setValue("fieldDouble", makeValueCopy(333.3)).ignore();
        dict->setValue("fieldBoolTrue", makeValueCopy(true)).ignore();
        dict->setValue("fieldBoolFalse", makeValueCopy(false)).ignore();
        dict->setValue("fieldString", makeValueCopy(std::string_view{"text_1"})).ignore();

        const Json::Value& jsonValue = dict->as<const serialization::JsonValueHolder&>().getThisJsonValue();

        ASSERT_EQ(jsonValue.type(), Json::ValueType::objectValue);

        ASSERT_EQ(jsonValue["fieldInt"].type(), Json::ValueType::intValue);
        ASSERT_EQ(jsonValue["fieldInt"].asInt(), 111);

        ASSERT_EQ(jsonValue["fieldSingle"].type(), Json::ValueType::realValue);
        ASSERT_EQ(jsonValue["fieldSingle"].asFloat(), 222.2f);

        ASSERT_EQ(jsonValue["fieldDouble"].type(), Json::ValueType::realValue);
        ASSERT_EQ(jsonValue["fieldDouble"].asDouble(), 333.3);

        ASSERT_EQ(jsonValue["fieldBoolTrue"].type(), Json::ValueType::booleanValue);
        ASSERT_TRUE(jsonValue["fieldBoolTrue"].asBool());

        ASSERT_EQ(jsonValue["fieldBoolFalse"].type(), Json::ValueType::booleanValue);
        ASSERT_FALSE(jsonValue["fieldBoolFalse"].asBool());

        ASSERT_EQ(jsonValue["fieldString"].type(), Json::ValueType::stringValue);
        ASSERT_EQ(jsonValue["fieldString"].asString(), "text_1");
    }

    /**
     */
    TEST(TestSerializationJsonMutability, SetOptional)
    {
        Ptr<RuntimeDictionary> dict = serialization::jsonCreateDictionary();

        dict->setValue("notNull", makeValueCopy(std::optional<unsigned>(77))).ignore();
        dict->setValue("null", makeValueCopy(std::optional<unsigned>(std::nullopt))).ignore();

        const Json::Value& jsonValue = dict->as<const serialization::JsonValueHolder&>().getThisJsonValue();
        ASSERT_EQ(jsonValue["notNull"].type(), Json::ValueType::uintValue);
        ASSERT_EQ(jsonValue["notNull"].asInt(), 77);
        ASSERT_EQ(jsonValue["null"].type(), Json::ValueType::nullValue);
    }

    /**
     */
    TEST(TestSerializationJsonMutability, SetCollection)
    {
        Ptr<RuntimeDictionary> dict = serialization::jsonCreateDictionary();

        std::vector<int> values{11, 22, 33};

        dict->setValue("values", makeValueCopy(std::move(values))).ignore();

        const Json::Value& jsonValue = dict->as<const serialization::JsonValueHolder&>().getThisJsonValue();
        const Json::Value& jsonCollectionValue = jsonValue["values"];
        ASSERT_EQ(jsonCollectionValue.type(), Json::ValueType::arrayValue);
        ASSERT_EQ(jsonCollectionValue.size(), 3);
        ASSERT_EQ(jsonCollectionValue[0].asInt(), 11);
        ASSERT_EQ(jsonCollectionValue[1].asInt(), 22);
        ASSERT_EQ(jsonCollectionValue[2].asInt(), 33);
    }

    /**
     */
    TEST(TestSerializationJsonMutability, SetDictionary)
    {
        Ptr<RuntimeDictionary> dict = serialization::jsonCreateDictionary();

        std::map<std::string, int> values{
            { "first", 11},
            {"second", 22},
            { "third", 33}
        };

        dict->setValue("values", makeValueCopy(std::move(values))).ignore();

        const Json::Value& jsonValue = dict->as<const serialization::JsonValueHolder&>().getThisJsonValue();
        const Json::Value& jsonObjectValue = jsonValue["values"];
        ASSERT_EQ(jsonObjectValue.type(), Json::ValueType::objectValue);
        ASSERT_EQ(jsonObjectValue.size(), 3);
        ASSERT_EQ(jsonObjectValue["first"].asInt(), 11);
        ASSERT_EQ(jsonObjectValue["second"].asInt(), 22);
        ASSERT_EQ(jsonObjectValue["third"].asInt(), 33);
    }

    /**
        Test: merge json values
     */
    TEST(TestSerializationJsonMutability, Merge)
    {
        using namespace my::serialization;

        std::string_view json1 =
            R"--(
            {
                "id": 111,
                "type": "object",
                "data_1": {
                    "id": 101,
                    "type": "number",
                    "prop1": 100,
                    "prop2": 200
                },
                "values": ["one", 2]
            }
        )--";

        std::string_view json2 =
            R"--(
            {
                "id_2": 222,
                "type": "object_2",
                "data_1": {
                    "id_x": 101,
                    "type_x": "number",
                    "prop3_x": 300,
                    "prop4_x": 400
                },
                
                "values": ["three", 4],

                "data_2": {
                    "id": 101,
                    "type": "number",
                    "prop1": 111,
                    "prop2": 222
                }
            }
        )--";

        const auto containsField = [](const Json::Value& jsonValue, std::string_view fieldName)
        {
            return jsonValue.find(fieldName.data(), fieldName.data() + fieldName.size()) != nullptr;
        };

        const RuntimeValuePtr value1 = *jsonParseString(json1);
        const RuntimeValuePtr value2 = *jsonParseString(json2);

        ASSERT_TRUE(RuntimeValue::assign(value1, value2, ValueAssignOption::MergeCollection));

        const Json::Value& jsonValue = value1->as<const JsonValueHolder&>().getThisJsonValue();

        ASSERT_TRUE(containsField(jsonValue, "id"));
        ASSERT_TRUE(containsField(jsonValue, "id_2"));
        ASSERT_TRUE(containsField(jsonValue, "type"));
        ASSERT_TRUE(containsField(jsonValue, "data_1"));
        ASSERT_TRUE(containsField(jsonValue, "data_2"));
        ASSERT_TRUE(containsField(jsonValue, "values"));

        {   // merged collection
            const Json::Value& valuesValue = jsonValue["values"];
            ASSERT_EQ(valuesValue.type(), Json::ValueType::arrayValue);
            ASSERT_EQ(valuesValue.size(), 4);
            ASSERT_EQ(valuesValue[0].asString(), "one");
            ASSERT_EQ(valuesValue[1].asInt(), 2);
            ASSERT_EQ(valuesValue[2].asString(), "three");
            ASSERT_EQ(valuesValue[3].asInt(), 4);
        }

        {   // merged sub object
            const Json::Value& data1Value = jsonValue["data_1"];
            ASSERT_TRUE(containsField(data1Value, "id"));
            ASSERT_TRUE(containsField(data1Value, "type"));
            ASSERT_TRUE(containsField(data1Value, "prop1"));
            ASSERT_TRUE(containsField(data1Value, "prop2"));
            ASSERT_TRUE(containsField(data1Value, "id_x"));
            ASSERT_TRUE(containsField(data1Value, "type_x"));
            ASSERT_TRUE(containsField(data1Value, "prop3_x"));
            ASSERT_TRUE(containsField(data1Value, "prop4_x"));
        }

        // Debug code: uncomment to see actual value1 (merged) content.
#if 0
        std::string buffer;
        io::InplaceStringWriter<char> writer{buffer};
        auto res = serialization::jsonWrite(writer, value1, JsonSettings{.pretty = true});
        if (!res)
        {
            std::cout << strings::toStringView(res.getError()->getMessage()) << std::endl;
        }

        std::cout << strings::toStringView(buffer) << std::endl;
#endif
    }

}  // namespace my::test
