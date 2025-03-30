// #my_engine_source_file

#pragma once
#include "my/rtti/rtti_impl.h"
#include "my/serialization/json.h"


namespace my::json_detail
{
    /**
     */
    class MY_ABSTRACT_TYPE JsonValueHolderImpl : public virtual IRefCounted,
                                                  public serialization::JsonValueHolder
    {
        MY_INTERFACE(my::json_detail::JsonValueHolderImpl, IRefCounted, serialization::JsonValueHolder)

    public:
        Json::Value& getRootJsonValue() final
        {
            return m_root ? m_root->getThisJsonValue() : getThisJsonValue();
        }

        const Json::Value& getRootJsonValue() const final
        {
            return m_root ? m_root->getThisJsonValue() : getThisJsonValue();
        }

        void setGetStringCallback(GetStringCallback callback) final
        {
            if (m_root)
            {
                m_root->setGetStringCallback(std::move(callback));
            }
            else
            {
                NAU_ASSERT(!m_getStringCallback);
                m_getStringCallback = std::move(callback);
            }
        }

        std::optional<std::string> transformString(std::string_view str)
        {
            auto& callback = m_root ? m_root->m_getStringCallback : m_getStringCallback;
            if (callback)
            {
                return callback(str);
            }

            return std::nullopt;
        }

        void setMutable(bool isMutable)
        {
            m_isMutable = isMutable;
        }
        

    protected:
        JsonValueHolderImpl()
        {
            m_jsonValue.emplace<Json::Value>();
        }

        JsonValueHolderImpl(Json::Value&& value)
        {
            m_jsonValue.emplace<Json::Value>(std::move(value));
        }

        JsonValueHolderImpl(Json::Value& value)
        {
            m_jsonValue.emplace<Json::Value*>(&value);
        }

        JsonValueHolderImpl(const my::Ptr<JsonValueHolderImpl>& root, Json::Value& value) :
            m_root(root)
        {
            NAU_FATAL(root);
            m_jsonValue.emplace<Json::Value*>(&value);
        }

        // Constructor dedicated to using only with null value representation
        JsonValueHolderImpl(const my::Ptr<JsonValueHolderImpl>& root) :
            m_root(root)
        {
            NAU_FATAL(root);
            m_jsonValue.emplace<Json::Value>();
        }

        Json::Value& getThisJsonValue() final
        {
            NAU_FATAL(!m_jsonValue.valueless_by_exception());

            if (eastl::holds_alternative<Json::Value>(m_jsonValue))
            {
                return eastl::get<Json::Value>(m_jsonValue);
            }

            NAU_FATAL(eastl::holds_alternative<Json::Value*>(m_jsonValue));
            return *eastl::get<Json::Value*>(m_jsonValue);
        }

        const Json::Value& getThisJsonValue() const final
        {
            NAU_FATAL(!m_jsonValue.valueless_by_exception());

            if (eastl::holds_alternative<Json::Value>(m_jsonValue))
            {
                return eastl::get<Json::Value>(m_jsonValue);
            }

            NAU_FATAL(eastl::holds_alternative<Json::Value*>(m_jsonValue));
            return *eastl::get<Json::Value*>(m_jsonValue);
        }

        my::Ptr<JsonValueHolderImpl> getRoot()
        {
            if (m_root)
            {
                return m_root;
            }

            return my::Ptr{this};
        }

        const my::Ptr<JsonValueHolderImpl> m_root;
        eastl::variant<Json::Value, Json::Value*> m_jsonValue;
        bool m_isMutable = true;
        GetStringCallback m_getStringCallback;
    };

    RuntimeValue::Ptr getValueFromJson(const my::Ptr<JsonValueHolderImpl>& root, Json::Value& jsonValue);

    RuntimeDictionary::Ptr createJsonDictionary(Json::Value&& jsonValue);
    
    RuntimeCollection::Ptr createJsonCollection(Json::Value&& jsonValue);

    RuntimeDictionary::Ptr wrapJsonDictionary(Json::Value& jsonValue);
    
    RuntimeCollection::Ptr wrapJsonCollection(Json::Value& jsonValue);

}  // namespace my::json_detail
