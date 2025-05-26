// #my_engine_source_file

#pragma once
#include <variant>
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
                MY_DEBUG_ASSERT(!m_getStringCallback);
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
            MY_FATAL(root);
            m_jsonValue.emplace<Json::Value*>(&value);
        }

        // Constructor dedicated to using only with null value representation
        JsonValueHolderImpl(const my::Ptr<JsonValueHolderImpl>& root) :
            m_root(root)
        {
            MY_FATAL(root);
            m_jsonValue.emplace<Json::Value>();
        }

        Json::Value& getThisJsonValue() final
        {
            MY_FATAL(!m_jsonValue.valueless_by_exception());

            if (std::holds_alternative<Json::Value>(m_jsonValue))
            {
                return std::get<Json::Value>(m_jsonValue);
            }

            MY_FATAL(std::holds_alternative<Json::Value*>(m_jsonValue));
            return *std::get<Json::Value*>(m_jsonValue);
        }

        const Json::Value& getThisJsonValue() const final
        {
            MY_FATAL(!m_jsonValue.valueless_by_exception());

            if (std::holds_alternative<Json::Value>(m_jsonValue))
            {
                return std::get<Json::Value>(m_jsonValue);
            }

            MY_FATAL(std::holds_alternative<Json::Value*>(m_jsonValue));
            return *std::get<Json::Value*>(m_jsonValue);
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
        std::variant<Json::Value, Json::Value*> m_jsonValue;
        bool m_isMutable = true;
        GetStringCallback m_getStringCallback;
    };

    RuntimeValuePtr getValueFromJson(const my::Ptr<JsonValueHolderImpl>& root, Json::Value& jsonValue);

    Ptr<RuntimeDictionary> createJsonDictionary(Json::Value&& jsonValue);
    
    Ptr<RuntimeCollection> createJsonCollection(Json::Value&& jsonValue);

    Ptr<RuntimeDictionary> wrapJsonDictionary(Json::Value& jsonValue);
    
    Ptr<RuntimeCollection> wrapJsonCollection(Json::Value& jsonValue);

}  // namespace my::json_detail
