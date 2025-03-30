// #my_engine_source_file
#include "my/utils/runtime_enum.h"

#include "my/diag/check.h"
#include "my/diag/logging.h"
#include "my/utils/string_utils.h"

// #include "my/string/string_utils.h"

namespace my::kernel_detail
{
    namespace
    {
        /**
         parse value
             "EnumValue = XXX" -> "EnumValue"
      */
        std::string_view parseSingleEnumEntry(std::string_view enumStr)
        {
            const auto pos = enumStr.find_first_of('=');
            if (pos == std::string_view::npos)
            {
                return strings::trim(enumStr);
            }

            return strings::trim(enumStr.substr(0, pos));
        }
    }  // namespace

    void EnumTraitsHelper::parseEnumDefinition(std::string_view enumDefinitionString, [[maybe_unused]] size_t itemCount, std::string_view* result)
    {
        MY_DEBUG_FATAL(result);
        MY_DEBUG_FATAL(itemCount > 0);
        MY_DEBUG_FATAL(!enumDefinitionString.empty());

        size_t index = 0;

        for (auto singleEnumString : strings::split(enumDefinitionString, std::string_view{","}))
        {
            MY_DEBUG_FATAL(index < itemCount);
            result[index++] = parseSingleEnumEntry(singleEnumString);
        }
    }

    std::string_view EnumTraitsHelper::toString(IEnumRuntimeInfo& enumInfo, int value)
    {
        const auto intValues = enumInfo.getIntValues();
        size_t index = 0;
        for (size_t count = intValues.size(); index < count; ++index)
        {
            if (intValues[index] == static_cast<int>(value))
            {
                break;
            }
        }

        if (index == intValues.size())
        {
            MY_DEBUG_FAILURE("Invalid enum ({}) int value ({})", enumInfo.getName(), value);
            return {};
        }

        const auto strValues = enumInfo.getStringValues();
        MY_DEBUG_FATAL(index < strValues.size(), "Invalid internal enum runtime info");

        return strValues[index];
    }

    Result<int> EnumTraitsHelper::parse(IEnumRuntimeInfo& enumInfo, std::string_view str)
    {
        const auto strValues = enumInfo.getStringValues();
        size_t index = 0;
        for (size_t count = strValues.size(); index < count; ++index)
        {
            if (strings::icaseEqual(str, strValues[index]))
            {
                break;
            }
        }

        if (index == strValues.size())
        {
            return MakeError("Invalid enum value ({})", str);
        }

        const auto intValues = enumInfo.getIntValues();
        MY_DEBUG_FATAL(index < intValues.size(), "Invalid internal enum runtime info ({})", str);

        return intValues[index];
    }

}  // namespace my::kernel_detail
