#include "lua_toolkit/lua_utils.h"
#include "lua_debugger_state.h"
#include "lua_variable_manager.h"
#include "my/rtti/rtti_impl.h"
#include "my/diag/logging.h"

namespace my::lua {

namespace {

/**
 */
class TableVariable final : public TableVariableBase
{
    MY_RTTI_CLASS(my::lua::TableVariable, TableVariableBase)

public:
    using TableVariableBase::TableVariableBase;
};

/**
 */
class FunctionVariable final : public ClosureVariableBase
{
    MY_RTTI_CLASS(my::lua::FunctionVariable, ClosureVariableBase)

public:
    using ClosureVariableBase::ClosureVariableBase;
};

/**
 */
class RefStorageVariableImpl final : public RefStorageVariable
{
    MY_RTTI_CLASS(my::lua::RefStorageVariableImpl, RefStorageVariable)

public:
    using RefStorageVariable::RefStorageVariable;

    bool PushChildOnStack(ChildVariableKey key) override
    {
        MY_DEBUG_ASSERT(key.IsIndexed());

        lua_State* const l = this->GetLua();
        DebuggerState::GetReference(l, static_cast<int>(key));

        if (lua_isnoneornil(l, -1))
        {
            lua_pop(l, 1);
            return false;
        }

        return true;
    }

    std::vector<VariableKeyValue> GetChildVariables(VariableManager&) override
    {
        MY_DEBUG_FAILURE("RefStorageVariable::GetChildVariables does not implemented (it does not makes any sense)");
        return {};
    }

    ChildVariableKey Ref(int stackIndex) override
    {
        lua_State* const l = this->GetLua();

        lua_pushvalue(l, stackIndex);
        const int refId = DebuggerState::KeepReference(l);
        return refId;
    }

    void Unref(int refId) override
    {
        DebuggerState::ReleaseReference(this->GetLua(), refId);
    }
};

}  // namespace

CompoundVariable::CompoundVariable(lua_State* l) :
    _lua(l)
{
}

lua_State* CompoundVariable::GetLua() const
{
    return _lua;
}

//-----------------------------------------------------------------------------
bool ClosureVariableBase::PushSelfOnStack()
{
    return true;
}

bool ClosureVariableBase::PushChildOnStack(ChildVariableKey key)
{
    if (!PushSelfOnStack())
    {
        return false;
    }

    lua_State* const l = this->GetLua();
    MY_DEBUG_ASSERT(lua_type(l, -1) == LUA_TFUNCTION);
    const char* const name = lua_getupvalue(l, -1, static_cast<int>(key));

    return name != nullptr;
}

std::vector<CompoundVariable::VariableKeyValue> ClosureVariableBase::GetChildVariables(VariableManager& manager)
{
    if (!PushSelfOnStack())
    {
        return {};
    }

    lua_State* const l = this->GetLua();
    MY_DEBUG_ASSERT(lua_type(l, -1) == LUA_TFUNCTION);
    std::vector<VariableKeyValue> variables;

    const UpValuesEnumerator enumerator{l, -1};
    for (auto upValue = enumerator.begin(), end = enumerator.end(); upValue != end; ++upValue)
    {
        variables.emplace_back(manager.CreateVariableFromLuaStack(l, -1, *this, ChildVariableKey(upValue.Index()), upValue.Name()));
    }

    return variables;
}

//-----------------------------------------------------------------------------

TableVariableBase::TableVariableBase(lua_State* l, int stackIndex) :
    CompoundVariable(l)
{
    // MY_DEBUG_ASSERT(lua_type(l, stackIndex) == LUA_TTABLE);

    guard_lstack(l);

    for (auto [keyIndex, _] : TableEnumerator{l, stackIndex})
    {
        // [-2]: key
        // [-1]: value

        const int keyType = lua_type(l, keyIndex);
        MY_DEBUG_ASSERT(keyType == LUA_TNUMBER || keyType == LUA_TSTRING);

        if (keyType == LUA_TNUMBER)
        {
            const int index = static_cast<int>(lua_tointeger(l, keyIndex));
            _indexedKeys.push_back(index);
        }
        else if (keyType == LUA_TSTRING)
        {
            size_t len;
            const char* const value = lua_tolstring(l, keyIndex, &len);
            _namedKeys.emplace_back(value, len);
        }
        else
        {
            
        }
    }
}

bool TableVariableBase::PushSelfOnStack()
{
    return true;
}

unsigned TableVariableBase::IndexedFieldsCount() const
{
    return static_cast<unsigned>(_indexedKeys.size());
}

unsigned TableVariableBase::NamedFieldsCount() const
{
    return static_cast<unsigned>(_namedKeys.size());
}

bool TableVariableBase::PushChildOnStack(ChildVariableKey key)
{
    if (!this->PushSelfOnStack())
    {
        return false;
    }
    lua_State* const l = this->GetLua();
    MY_DEBUG_ASSERT(lua_type(l, -1) == LUA_TTABLE);
    key.Push(l);

    lua_gettable(l, -2);
    lua_remove(l, -2);

    return true;
}

std::vector<CompoundVariable::VariableKeyValue> TableVariableBase::GetChildVariables(VariableManager& manager)
{
    if (!this->PushSelfOnStack())
    {
        return {};
    }

    lua_State* const l = this->GetLua();
    guard_lstack(l);

    std::vector<VariableKeyValue> variables;

    MY_DEBUG_ASSERT(lua_type(l, -1) == LUA_TTABLE);

    for (int index : _indexedKeys)
    {
        lua_pushinteger(l, index);
        lua_gettable(l, -2);
        variables.push_back(manager.CreateVariableFromLuaStack(l, -1, *this, index));
        lua_pop(l, 1);
    }

    for (const std::string& name : _namedKeys)
    {
        lua_pushlstring(l, name.data(), name.length());
        lua_gettable(l, -2);
        variables.push_back(manager.CreateVariableFromLuaStack(l, -1, *this, ChildVariableKey{name}));
        lua_pop(l, 1);
    }

    return variables;
}

//-----------------------------------------------------------------------------
std::unique_ptr<RefStorageVariable> RefStorageVariable::Create(lua_State* l)
{
    return std::make_unique<RefStorageVariableImpl>(l);
}

//-----------------------------------------------------------------------------
VariableRegistration::VariableRegistration(VariableManager& manager, unsigned id) :
    _manager(&manager),
    _id(id)
{
}

VariableRegistration::VariableRegistration(VariableRegistration&& other) :
    _manager(other._manager),
    _id(other._id)
{
    other._manager = nullptr;
    other._id = 0;
}

VariableRegistration::~VariableRegistration()
{
    this->Reset();
}

void VariableRegistration::Reset()
{
    if (_manager != nullptr && _id > 0)
    {
        _manager->RemoveVariableAndChildren(_id);
        _manager = nullptr;
        _id = 0;
    }
}

VariableRegistration& VariableRegistration::operator=(VariableRegistration&& other)
{
    this->Reset();
    MY_DEBUG_ASSERT(_manager == nullptr && _id == 0);
    std::swap(_manager, other._manager);
    std::swap(_id, other._id);

    return *this;
}

VariableRegistration::operator bool() const
{
    return _manager != nullptr && _id > 0;
}

VariableRegistration::operator unsigned() const
{
    return this->VariableId();
}

unsigned VariableRegistration::VariableId() const
{
    MY_DEBUG_ASSERT(static_cast<bool>(*this));
    return _id;
}

CompoundVariable& VariableRegistration::operator*() const
{
    return this->GetVariable();
}

CompoundVariable* VariableRegistration::operator->() const
{
    return &this->GetVariable();
}

CompoundVariable& VariableRegistration::GetVariable() const
{
    if (!_variablePtr)
    {
        auto entry = _manager->FindCompoundVariableEntry(_id);
        MY_DEBUG_ASSERT(entry);
        MY_DEBUG_ASSERT(entry->variable);
        _variablePtr = entry->variable.get();
    }

    return *_variablePtr;
}

//-----------------------------------------------------------------------------
VariableManager::CompoundVariableEntry::CompoundVariableEntry(unsigned varId, unsigned parentVarId, ChildVariableKey varKey, CompoundVariablePtr value) :
    id(varId),
    parentId(parentVarId),
    key(std::move(varKey)),
    variable(std::move(value))
{
}

VariableManager::CompoundVariableEntry::CompoundVariableEntry(unsigned varId, CompoundVariablePtr value) :
    id(varId),
    variable(std::move(value))
{
}

//-----------------------------------------------------------------------------
CompoundVariable::VariableKeyValue VariableManager::CreateVariableFromLuaStack(lua_State* l, int stackIndex, CompoundVariable& parent, ChildVariableKey childKey, std::string_view overrideVariableName, VariableRegistration* varRegistaration)
{
    CompoundVariable::VariableKeyValue keyValue;

    dap::Variable& variable = keyValue.second;
    if (!overrideVariableName.empty())
    {
        variable.name = overrideVariableName;
    }
    else if (childKey)
    {
        variable.name = childKey.AsString();
    }

    // Ключ (имя/индекс) дочерней переменной вычисляется только если нужно.
    // Составные значения (таблицы, функции) могут храниться как ссылки (в случае watch или использования repl), но простые значения вычисляются сразу.
    const auto GetChildKey = [&]() -> ChildVariableKey&
    {
        auto& key = keyValue.first;

        if (!key)
        {
            if (auto refStorage = parent.as<RefStorageVariable*>(); refStorage)
            {
                MY_DEBUG_ASSERT(!childKey);
                key = refStorage->Ref(stackIndex);
            }
            else
            {
                key = std::move(childKey);
            }
            MY_DEBUG_ASSERT(key);
        }

        return (key);
    };

    guard_lstack(l);

    variable.presentationHint.emplace();
    variable.presentationHint->kind = "property";
    variable.presentationHint->visibility = "public";

    const int valueType = lua_type(l, stackIndex);

    if (valueType == LUA_TNIL)
    {
        variable.value = "nil";
        variable.type = "nil";
    }
    else if (valueType == LUA_TBOOLEAN)
    {
        const auto value = lua_toboolean(l, stackIndex);
        variable.value = value ? "True" : "False";
        variable.type = "boolean";
    }
    else if (valueType == LUA_TLIGHTUSERDATA)
    {
        variable.value = "[USER_POINTER]";
        variable.type = "pointer";
    }
    else if (valueType == LUA_TNUMBER)
    {
        const auto value = lua_tonumber(l, -1);
        variable.value = std::to_string(value);
        variable.type = "number";
    }
    else if (valueType == LUA_TSTRING)
    {
        size_t len;
        const char* const value = lua_tolstring(l, -1, &len);
        variable.value.assign(value, len);
        variable.type = "string";
    }
    else if (valueType == LUA_TTABLE)
    {
        auto tableVariable = std::make_unique<TableVariable>(l, -1);

        if (tableVariable->NamedFieldsCount() > 0 || tableVariable->IndexedFieldsCount() > 0)
        {
            CompoundVariableEntry* const parentEntry = this->FindCompoundVariableEntry(parent);
            MY_DEBUG_ASSERT(parentEntry);
            variable.type = tableVariable->NamedFieldsCount() > 0 ? "table" : "array";
            variable.indexedVariables = tableVariable->IndexedFieldsCount();
            variable.namedVariables = tableVariable->NamedFieldsCount();
            variable.variablesReference = _compoundVariables.emplace_back(GetNextVariableId(), parentEntry->id, GetChildKey(), std::move(tableVariable)).id;
        }
        else
        {
            variable.type = "table";
        }

        variable.value = "{}";
    }
    else if (valueType == LUA_TFUNCTION)
    {
        lua_Debug funcInfo;

        // когда указан '>' lua_getinfo использует значение с вершины стека (убирая его): https://www.lua.org/manual/5.2/manual.html#lua_getinfo
        lua_pushvalue(l, stackIndex);
        const bool infoReady = lua_getinfo(l, ">unS", &funcInfo) != 0;

        if (infoReady)
        {
            std::string_view what{funcInfo.what};

            if (what == "Lua")
            {
                if (funcInfo.nups > 0)
                {
                    CompoundVariableEntry* const parentEntry = this->FindCompoundVariableEntry(parent);
                    variable.variablesReference = _compoundVariables.emplace_back(GetNextVariableId(), parentEntry->id, GetChildKey(), std::make_unique<FunctionVariable>(l)).id;
                    variable.value = fmt::format("(), [{}]", funcInfo.nups);
                    variable.type = "closure";
                }
                else
                {
                    variable.value = "()";
                    variable.type = "function";
                }
            }
            else if (what == "C")
            {
                variable.value = "() c/c++";
                variable.type = "cfunction";
            }
            else
            {
                variable.value = "()";
                variable.type = "function";
            }
        }
        else
        {
            variable.value = "()";
            variable.type = "function";
        }
    }
    else if (valueType == LUA_TUSERDATA)
    {
        variable.value = "[USERDATA]";
        variable.type = "pointer";
    }
    else if (valueType == LUA_TTHREAD)
    {
        variable.value = "[THREAD]";
        variable.type = "thread";
    }
    else
    {
        variable.value = "[UNKNOWN]";
        variable.type = "unknown";
    }

    ChildVariableKey& key = keyValue.first;

    if (!key)
    {
        key = std::move(childKey);
    }

    if (variable.name.empty())
    {
        if (!overrideVariableName.empty())
        {
            variable.name = overrideVariableName;
        }
        else if (key)
        {
            variable.name = key.IsIndexed() ? std::to_string(static_cast<int>(key)) : key.GetName();
        }
    }

    if (varRegistaration)
    {
        if (variable.variablesReference != 0)
        {
            *varRegistaration = VariableRegistration{*this, variable.variablesReference};
        }
        else
        {
            varRegistaration->Reset();
        }
    }

    MY_DEBUG_ASSERT(variable.variablesReference == 0 || key);

    return keyValue;
}

VariableRegistration VariableManager::RegisterVariableRoot(CompoundVariablePtr variable)
{
    MY_DEBUG_ASSERT(variable);
    if (!variable)
    {
        return {};
    }

    const unsigned variableRefId = _compoundVariables.emplace_back(GetNextVariableId(), std::move(variable)).id;
    return VariableRegistration{*this, variableRefId};
}

std::vector<dap::Variable> VariableManager::GetVariables(const dap::VariablesArguments& args)
{
    CompoundVariableEntry* const varEntry = FindCompoundVariableEntry(args.variablesReference);
    if (varEntry == nullptr)
    {
        return {};
    }

    if (!varEntry->children)
    {
        // Ищет переменную у которой parent == parentId. Поиск всегда начинается с "листа" дерева varEntry.
        const auto FindNextWithParent = [&](unsigned parentId) -> CompoundVariableEntry*
        {
            CompoundVariableEntry* entry = varEntry;

            while (entry && entry->parentId != parentId)
            {
                if (entry->parentId == 0)
                {
                    return nullptr;
                }
                entry = FindCompoundVariableEntry(entry->parentId);
            }

            return entry;
        };

        // Необходимо запушить на стек lua значение связанное с varEntry.
        // Для этого поочередно пушим на стек родительские переменные (начиная с самого корня)
        // и далее каждая родительская переменная будет пушить значение по имени/индексу поля.
        // Только parent "знает" как поместить значение связанное с конкретным ключом на стек.
        // myLocal.myTable.myField[0]:
        // - находим корень:  0 -> myField -> myTable -> myLocal -> (root)
        // - находим вхождение для myLocal (т.е. переменную для которой parent - ранее найденный root)
        // - parent = (root), field = (myLocal),  (root) пушит (field:myLocal) на стек;
        // - находим следующее поле: myTable (т.е. переменную для которой parent - myLocal)
        // - parent = (myLocal), field = (myTable),  (parent:myLocal) пушит (field:myTable) на стек;
        // - и т.д. по цепочке пока в конечном счёте на стеке не окажется целевое значение.
        CompoundVariableEntry* parent = FindNextWithParent(0);  // root
        CompoundVariableEntry* field = nullptr;
        do
        {
            if (parent == varEntry)
            {
                break;
            }

            field = FindNextWithParent(parent->id);
            MY_DEBUG_ASSERT(field);

            parent->variable->PushChildOnStack(field->key);
            parent = field;
        }
        while (field != varEntry);

        varEntry->children.emplace(varEntry->variable->GetChildVariables(*this));
    }

    std::vector<dap::Variable> variables;
    variables.reserve(varEntry->children->size());

    const std::optional<bool> filterIndexed = !args.filter.empty() ? std::optional<bool>{args.filter == "indexed"} : std::nullopt;

    for (const auto& [key, variable] : *varEntry->children)
    {
        if (!filterIndexed.has_value())
        {
            variables.emplace_back(variable);
        }
        else if (*filterIndexed == key.IsIndexed())
        {
            variables.emplace_back(variable);
        }
    }

    return variables;
}

unsigned VariableManager::GetNextVariableId()
{
    return ++_nextVariableId;
}

VariableManager::CompoundVariableEntry* VariableManager::FindCompoundVariableEntry(unsigned variableId)
{
    auto iter = std::find_if(_compoundVariables.begin(), _compoundVariables.end(), [variableId](const CompoundVariableEntry& entry)
    {
        return entry.id == variableId;
    });

    return iter != _compoundVariables.end() ? &(*iter) : nullptr;
}

VariableManager::CompoundVariableEntry* VariableManager::FindCompoundVariableEntry(const CompoundVariable& variable)
{
    auto iter = std::find_if(_compoundVariables.begin(), _compoundVariables.end(), [&variable](const CompoundVariableEntry& entry)
    {
        return entry.variable.get() == &variable;
    });

    return iter != _compoundVariables.end() ? &(*iter) : nullptr;
}

void VariableManager::RemoveVariableAndChildren(unsigned variableId)
{
    const auto UnrefIfNeeded = [](CompoundVariable& parentVar, const ChildVariableKey& key)
    {
        if (auto refStorage = parentVar.as<RefStorageVariable*>(); refStorage)
        {
            refStorage->Unref(static_cast<int>(key));
        }
    };

    std::list<CompoundVariableEntry> removedVariables;

    if (auto var = std::find_if(_compoundVariables.begin(), _compoundVariables.end(), [variableId](const CompoundVariableEntry& e)
    {
        return e.id == variableId;
    });
        var != _compoundVariables.end())
    {
        removedVariables.splice(removedVariables.begin(), _compoundVariables, var);

        auto parent = var->parentId != 0 ? std::find_if(_compoundVariables.begin(), _compoundVariables.end(), [parentId = var->parentId](const CompoundVariableEntry& e)
        {
            return e.id == parentId;
        })
                                         : _compoundVariables.end();
        if (parent != _compoundVariables.end())
        {
            UnrefIfNeeded(*parent->variable, var->key);
        }
    }
    else
    {
        mylog_warn("Attempt to remove unexistent variable");
        return;
    }

    bool anyRemoved = false;
    do
    {
        anyRemoved = false;

        for (auto iter = _compoundVariables.begin(), end = _compoundVariables.end(); iter != end;)
        {
            auto parent = std::find_if(removedVariables.begin(), removedVariables.end(), [parentId = iter->parentId](const CompoundVariableEntry& removedEntry)
            {
                return removedEntry.id == parentId;
            });
            if (parent != removedVariables.end())
            {
                UnrefIfNeeded(*parent->variable, iter->key);

                auto temp = iter++;
                removedVariables.splice(removedVariables.begin(), _compoundVariables, temp);
                anyRemoved = true;
            }
            else
            {
                ++iter;
            }
        }
    }
    while (anyRemoved);

    removedVariables.clear();
}

}  // namespace my::lua
