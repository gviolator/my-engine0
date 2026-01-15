// #my_engine_source_file
#pragma once

#include "my/diag/error.h"
#include "my/diag/source_info.h"
#include "my/meta/attribute.h"
#include "my/utils/result.h"

#include <format>


namespace my::serialization {

/**
 */
enum class TypeCoercion
{
    Default,
    Allow,
    Strict
};

/**
 */
class MY_ABSTRACT_TYPE SerializationError : public DefaultError<>
{
    MY_ERROR(my::serialization::SerializationError, my::DefaultError<>)
public:
    SerializationError(const diag::SourceInfo& sourceInfo, std::string message) :
        DefaultError<>(sourceInfo, std::move(message))
    {
    }
};

/**
 */
class RequiredFieldMissedError : public SerializationError
{
    MY_ERROR(my::serialization::RequiredFieldMissedError, SerializationError)

public:
    RequiredFieldMissedError(diag::SourceInfo sourceInfo, std::string typeName, std::string fieldName) :
        SerializationError(sourceInfo, makeMessage(typeName, fieldName)),
        m_typeName(std::move(typeName)),
        m_fieldName(std::move(fieldName))
    {
    }

    std::string_view getTypeName() const
    {
        return m_typeName;
    }

    std::string_view getFieldName() const
    {
        return m_fieldName;
    }

private:
    static std::string makeMessage(std::string_view type, std::string_view field)
    {
        std::string message = std::format("Required field ({}.{}) missed", type, field);
        return std::string{message.data(), message.size()};
    }

    const std::string m_typeName;
    const std::string m_fieldName;
};

/**
 */
class TypeMismatchError : public SerializationError
{
    MY_ERROR(my::serialization::TypeMismatchError, SerializationError)

public:
    TypeMismatchError(diag::SourceInfo sourceInfo, std::string expectedTypeName, std::string actualTypeName) :
        SerializationError(sourceInfo, makeMessage(expectedTypeName, actualTypeName)),
        m_expectedTypeName(std::move(expectedTypeName)),
        m_actualTypeName(std::move(actualTypeName))
    {
    }

    std::string_view getExpectedTypeName() const
    {
        return m_expectedTypeName;
    }

    std::string_view getActualTypeName() const
    {
        return m_actualTypeName;
    }

private:
    static std::string makeMessage(std::string_view expectedType, std::string_view actualType)
    {
        std::string message = std::format("Expected type(category):({}), but:({})", expectedType, actualType);
        return std::string{message.data(), message.size()};
    }

    const std::string m_expectedTypeName;
    const std::string m_actualTypeName;
};

/**
 */
class NumericOverflowError : public SerializationError
{
    MY_ERROR(my::serialization::NumericOverflowError, SerializationError)
public:
    using SerializationError::SerializationError;

    NumericOverflowError(diag::SourceInfo sourceInfo) :
        SerializationError(sourceInfo, "Numeric Overflow")
    {
    }
};

class EndOfStreamError : public SerializationError
{
    MY_ERROR(my::serialization::EndOfStreamError, SerializationError)
public:
    using SerializationError::SerializationError;

    EndOfStreamError(diag::SourceInfo sourceInfo) :
        SerializationError(sourceInfo, "Unexpected end of stream")
    {
    }
};

MY_DEFINE_ATTRIBUTE(RequiredFieldAttribute, "my.serialization.required_field", meta::AttributeOptionsNone)
MY_DEFINE_ATTRIBUTE(IgnoreEmptyFieldAttribute, "my.serialization.ignore_empty_field", meta::AttributeOptionsNone)

}  // namespace my::serialization
