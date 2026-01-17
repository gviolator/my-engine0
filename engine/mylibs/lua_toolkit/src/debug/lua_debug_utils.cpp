// #my_engine_source_file
#include "lua_debug_utils.h"
#include "lua_debugger_state.h"
#include "lua_toolkit/lua_utils.h"
#include "my/utils/scope_guard.h"

#include <regex>

namespace my::lua {

namespace {

inline std::string_view SubMatchToStringView(const std::sub_match<std::string_view::iterator>& sm)
{
    return sm.length() == 0 ? std::string_view{} : std::string_view{&(*sm.first), static_cast<size_t>(sm.length())};
}

/*
 */
inline std::string MakeExpressionString(std::string_view expression, int stackLevel)
{
    const auto MakeGetStackLocalCall = [](int stackLevel, std::string_view name) -> std::string
    {
        return fmt::format("{}.GetStackLocal({}, \"{}\")", DebuggerState::kFieldName_DebuggerState, stackLevel, name);
    };

    std::string result;
    std::match_results<std::string_view::iterator> match;

    const std::regex stackLocalRe{"\\$([A-Za-z0-9_]+)", std::regex_constants::ECMAScript};
    std::string_view str = expression;
    std::string_view suffix = expression;

    while (std::regex_search(str.begin(), str.end(), match, stackLocalRe))
    {
        if (!match.empty())
        {
            std::string_view prefix = SubMatchToStringView(match.prefix());
            suffix = SubMatchToStringView(match.suffix());

            std::string_view localName = SubMatchToStringView(match[1]);
            result = result
                         .append(prefix.data(), prefix.size())
                         .append(MakeGetStackLocalCall(stackLevel, localName));
        }

        if (const size_t len = match.suffix().length(); len > 0)
        {
            str = std::string_view{&(*match.suffix().first), len};
        }
        else
        {
            break;
        }
    }

    if (!suffix.empty())
    {
        result.append(suffix.data(), suffix.length());
    }

    return result;
}

inline std::string_view GetReplExpression(std::string_view expr)
{
    std::match_results<std::string_view::iterator> match;
    const std::regex re{"^\\s*#\\s*", std::regex_constants::ECMAScript};

    if (std::regex_search(expr.begin(), expr.end(), match, re))
    {
        auto v = SubMatchToStringView(match[0]);
        return std::string_view{expr.data() + v.size(), expr.size() - v.size()};
    }

    return {};
}

inline bool NeedToResolveDebugExpression(std::string_view expression)
{
    return expression.find_first_of("$") != std::string_view::npos;
}

}  // namespace

Result<> EvaluateExpression(std::string_view expression, lua_State* l, int stackLevel, EvaluateMode evalMode)
{
    std::string chunkExpression;

    // если строка начинается с '#', то replExpression будет = строке без префикса, в противном случае replExpression = пустая строка, т.е. нужно использовать выражение с 'return ...'
    std::string_view replExpression = evalMode == EvaluateMode::Repl ? GetReplExpression(expression) : std::string_view{};
    std::string_view actualExpression;

    if (replExpression.empty())
    {
        actualExpression = expression;
        chunkExpression = "return ";
    }
    else
    {
        actualExpression = replExpression;
    }

    if (NeedToResolveDebugExpression(actualExpression))
    {
        std::string resolvedExpression = MakeExpressionString(actualExpression, stackLevel);
        chunkExpression += std::move(resolvedExpression);
    }
    else
    {
        chunkExpression.append(actualExpression.data(), actualExpression.size());
    }

    return lua::execute(l, chunkExpression, 1, "eval");

    //if (std::string error = Lua::LoadBuffer(l, chunkExpression, "eval"); !error.empty())
    //{
    //    return MakeError(error);
    //}

    //if (lua_pcall(l, 0, 1, 0) != 0)
    //{
    //    scope_on_leave
    //    {
    //        lua_pop(l, 1);
    //    };

    //    size_t len;
    //    const char* const message = lua_tolstring(l, -1, &len);
    //    return MakeError(std::string{message, len});
    //}

    return kResultSuccess;
}

}  // namespace my::lua
