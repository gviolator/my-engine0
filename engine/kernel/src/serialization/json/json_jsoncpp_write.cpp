// #my_engine_source_file

#include <json/writer.h>

#include "my/serialization/json.h"

namespace my::serialization
{
    namespace
    {

        Json::StreamWriter& getJsonWriter(JsonSettings settings)
        {
            using namespace ::Json;

            static thread_local std::unique_ptr<StreamWriter> writer;
            static thread_local std::unique_ptr<StreamWriter> prettyWriter;

            const auto prepareWriter = [](std::unique_ptr<StreamWriter>& w, JsonSettings settings) -> StreamWriter&
            {
                if(!w)
                {
                    Json::StreamWriterBuilder wbuilder;
                    if(settings.pretty)
                    {
                        wbuilder["indentation"] = "\t";
                    }
                    else
                    {
                        wbuilder["indentation"] = "";
                    }

                    w = std::unique_ptr<Json::StreamWriter>(wbuilder.newStreamWriter());
                }

                return *w;
            };

            return settings.pretty ? prepareWriter(prettyWriter, settings) : prepareWriter(writer, settings);
        }

        class WriterStreambuf final : public std::streambuf
        {
        public:
            WriterStreambuf(io::IStream& writer) :
                m_writer(writer)
            {
            }

            std::streamsize xsputn(const char_type* s, std::streamsize n) override
            {
                static_assert(sizeof(char_type) == sizeof(std::byte));
                m_writer.write(reinterpret_cast<const std::byte*>(s), n).ignore();
                return n;
            };

            int_type overflow(int_type ch) override
            {
                m_writer.write(reinterpret_cast<const std::byte*>(&ch), 1).ignore();
                return 1;
            }

        private:
            io::IStream& m_writer;
        };

        void makeJsonPrimitiveValue(Json::Value& jValue, const PrimitiveValue& value)
        {
            if(auto integer = value.as<const IntegerValue*>(); integer)
            {
                if(integer->isSigned())
                {
                    jValue = Json::Value(integer->getInt64());
                }
                else
                {
                    jValue = Json::Value(integer->getUint64());
                }
            }
            else if(auto floatPoint = value.as<const FloatValue*>(); floatPoint)
            {
                if(floatPoint->getBitsCount() == sizeof(double))
                {
                    jValue = Json::Value(floatPoint->getDouble());
                }
                else
                {
                    jValue = Json::Value(floatPoint->getSingle());
                }
            }
            else if(auto str = value.as<const StringValue*>(); str)
            {
                auto text = str->getString();
                jValue = Json::Value(text.data(), text.data() + text.size());
            }
            else if(auto boolValue = value.as<const BooleanValue*>(); boolValue)
            {
                jValue = Json::Value(boolValue->getBool());
            }
            else
            {
                // Halt("Unknown primitive type for json serialization");
            }
        }

        Result<> makeJsonValue(Json::Value& jValue, const RuntimeValuePtr& value, const JsonSettings& settings)
        {
            if(OptionalValue* const optionalValue = value->as<OptionalValue*>())
            {
                if(optionalValue->hasValue())
                {
                    return makeJsonValue(jValue, optionalValue->getValue(), settings);
                }

                jValue = Json::Value(Json::nullValue);
                return {};
            }

            if(RuntimeValueRef* const refValue = value->as<RuntimeValueRef*>())
            {
                const auto referencedValue = refValue->getValue();
                if(referencedValue)
                {
                    return makeJsonValue(jValue, referencedValue, settings);
                }

                jValue = Json::Value(Json::nullValue);
                return {};
            }

            if(const PrimitiveValue* const primitiveValue = value->as<const PrimitiveValue*>(); primitiveValue)
            {
                makeJsonPrimitiveValue(jValue, *primitiveValue);
            }
            else if(ReadonlyCollection* const collection = value->as<ReadonlyCollection*>())
            {
                jValue = Json::Value(Json::arrayValue);

                for(size_t i = 0, sz = collection->getSize(); i < sz; ++i)
                {
                    if(auto result = makeJsonValue(jValue[jValue.size()], collection->getAt(i), settings); !result)
                    {
                        return result;
                    }
                }
            }
            else if(ReadonlyDictionary* const obj = value->as<ReadonlyDictionary*>())
            {
                jValue = Json::Value(Json::objectValue);

                for(size_t i = 0, sz = obj->getSize(); i < sz; ++i)
                {
                    auto key = obj->getKey(i);
                    auto member = obj->getValue(key);

                    if(!settings.writeNulls)
                    {
                        if(OptionalValue* const optionalValue = member->as<OptionalValue*>())
                        {
                            if(!optionalValue->hasValue())
                            {
                                continue;
                            }
                        }
                        else if(const RuntimeValueRef* refValue = member->as<const RuntimeValueRef*>())
                        {
                            if(!static_cast<bool>(refValue->getValue()))
                            {
                                continue;
                            }
                        }
                    }

                    if(auto result = makeJsonValue(jValue[key.data()], member, settings); !result)
                    {
                        return result;
                    }
                }
            }

            return {};
        }

    }  // namespace

    Result<> jsonWrite(io::IStream& writer, const Json::Value& value, JsonSettings settings)
    {
        MY_DEBUG_ASSERT(writer.canWrite());

        WriterStreambuf buf{writer};
        std::ostream stream(&buf);
        getJsonWriter(settings).write(value, &stream);

        return ResultSuccess;
    }

    Result<> jsonWrite(io::IStream& writer, const RuntimeValuePtr& value, JsonSettings settings)
    {
        MY_DEBUG_ASSERT(writer.canWrite());

        Json::Value root;
        if(auto result = makeJsonValue(root, value, settings); !result)
        {
            return result;
        }

        return jsonWrite(writer, root, settings);
    }

    Result<> runtimeApplyToJsonValue(Json::Value& jsonValue, const RuntimeValuePtr& runtimeValue, JsonSettings settings)
    {
        return makeJsonValue(jsonValue, runtimeValue, settings);
    }


    Json::Value runtimeToJsonValue(const RuntimeValuePtr& value, JsonSettings settings)
    {
        Json::Value root;
        makeJsonValue(root, value, settings).ignore();
        return root;
    }

}  // namespace my::serialization

