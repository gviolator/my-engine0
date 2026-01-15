// #my_engine_source_file
#pragma once

#include "lua_toolkit/lua_header.h"
#include "my/utils/result.h"

#include <string_view>

namespace my::lua {

/*
 *
 */
enum class EvaluateMode
{
    Watch,
    Repl
};

/*!
    ��������� lua ��������� �������� � expression, ��������� �������� �� ����.
    ������� ��������� ����������� ��� ��������� ���� $name.
    $ - �������� ������������� �������� ��������� ���������� ������������� �� �����:
        $name.myField "�����������" � '__my_DebugSession.GetStackLocal(stackLevel, "name").myField'.

    ���� evalMode = EvaluateMode � ������ ������������ � #, �� � ���������� ����� (��� '#') ����� ��������� "��� ����", � ��������� ����� ��������� ��� nil.
    � ��������� ������ ������� ����������� ������ = "return [expression]" .

    \param expression
    \param l
    \param stackLevel
    \param evalMode


*/
Result<> EvaluateExpression(std::string_view expression, lua_State* l, int stackLevel, EvaluateMode evalMode);

}  // namespace my::lua
