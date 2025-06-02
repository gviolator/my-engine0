// #my_engine_source_file

#include "my/meta/runtime_attribute.h"

#include "my/diag/assert.h"
#include "my/meta/attribute.h"

namespace my::meta
{
    void RuntimeAttributeContainer::setupUniqueKeys()
    {
        if (m_attributes.empty())
        {
            return;
        }

        m_uniqueKeys.reserve(m_attributes.size());
        std::transform(m_attributes.begin(), m_attributes.end(), std::back_inserter(m_uniqueKeys), [](const AttributeEntry& entry)
        {
            return entry.first;
        });

        std::sort(m_uniqueKeys.begin(), m_uniqueKeys.end());
        m_uniqueKeys.erase(std::unique(m_uniqueKeys.begin(), m_uniqueKeys.end()), m_uniqueKeys.end());
    }

    size_t RuntimeAttributeContainer::getSize() const
    {
        return m_uniqueKeys.size();
    }

    bool RuntimeAttributeContainer::containsAttribute(std::string_view key) const
    {
        return std::find(m_uniqueKeys.begin(), m_uniqueKeys.end(), key) != m_uniqueKeys.end();
    }

    std::string_view RuntimeAttributeContainer::getKey(size_t index) const
    {
        MY_FATAL(index < m_uniqueKeys.size());
        return m_uniqueKeys[index];
    }

    RuntimeValuePtr RuntimeAttributeContainer::getValue(std::string_view attributeKey) const
    {
        auto value = std::find_if(m_attributes.begin(), m_attributes.end(), [attributeKey](const AttributeEntry& entry)
        {
            return entry.first == attributeKey;
        });

        return value != m_attributes.end() ? value->second : nullptr;
    }

    std::vector<RuntimeValuePtr> RuntimeAttributeContainer::getAllValues(std::string_view key) const
    {
        std::vector<RuntimeValuePtr> values;

        for (const auto& [attribKey, attribValue] : m_attributes)
        {
            if (attribKey == key)
            {
                values.push_back(attribValue);
            }
        }

        return values;
    }

}  // namespace my::meta