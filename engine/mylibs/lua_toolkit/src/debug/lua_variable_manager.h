// #my_engine_source_file
#pragma once

#include "lua_internals.h"
#include "lua_toolkit/lua_header.h"
#include "my/debug/dap/dap.h"
#include "my/rtti/rtti_object.h"


#include <utility>
#include <memory>
#include <optional>
#include <vector>


namespace my::lua {

class VariableManager;

/**

*/
class MY_ABSTRACT_TYPE CompoundVariable : public IRttiObject
{
    MY_INTERFACE(my::lua::CompoundVariable, IRttiObject);
public:
    
    using VariableKeyValue = std::pair<ChildVariableKey, dap::Variable>;

    CompoundVariable(lua_State* l);

    virtual ~CompoundVariable() = default;

    virtual bool PushChildOnStack(ChildVariableKey key) = 0;

    virtual std::vector<VariableKeyValue> GetChildVariables(VariableManager&) = 0;

    lua_State* GetLua() const;

private:
    lua_State* const _lua;
};

using CompoundVariablePtr = std::unique_ptr<CompoundVariable>;

/*
 *
 */
class MY_ABSTRACT_TYPE ClosureVariableBase : public CompoundVariable
{
    MY_INTERFACE(my::lua::ClosureVariableBase, CompoundVariable);

public:
    bool PushChildOnStack(ChildVariableKey key) final;

    std::vector<VariableKeyValue> GetChildVariables(VariableManager&) final;

protected:
    using CompoundVariable::CompoundVariable;

    virtual bool PushSelfOnStack();
};

/*
 *
 */
class MY_ABSTRACT_TYPE TableVariableBase : public CompoundVariable
{
    MY_INTERFACE(my::lua::TableVariableBase, CompoundVariable);

public:
    TableVariableBase(lua_State*, int stackIndex);

    unsigned IndexedFieldsCount() const;

    unsigned NamedFieldsCount() const;

    bool PushChildOnStack(ChildVariableKey key) final;

    std::vector<VariableKeyValue> GetChildVariables(VariableManager&) final;

protected:
    virtual bool PushSelfOnStack();

private:
    std::vector<int> _indexedKeys;
    std::vector<std::string> _namedKeys;
};

/*
 */
class MY_ABSTRACT_TYPE RefStorageVariable : public CompoundVariable
{
    MY_INTERFACE(my::lua::TableVariableBase, CompoundVariable);

public:
    static std::unique_ptr<RefStorageVariable> Create(lua_State*);

    using CompoundVariable::CompoundVariable;

    virtual ChildVariableKey Ref(int stackIndex) = 0;

    virtual void Unref(int) = 0;
};

/**

*/
class [[nodiscard]] VariableRegistration
{
public:
    VariableRegistration() = default;

    ~VariableRegistration();

    VariableRegistration(VariableRegistration&&);

    VariableRegistration& operator=(VariableRegistration&&);

    explicit operator bool() const;

    operator unsigned() const;

    unsigned VariableId() const;

    CompoundVariable& operator*() const;

    CompoundVariable* operator->() const;

    void Reset();

private:
    VariableRegistration(VariableManager&, unsigned);

    CompoundVariable& GetVariable() const;

    VariableManager* _manager = nullptr;
    unsigned _id = 0;
    mutable CompoundVariable* _variablePtr = nullptr;

    friend VariableManager;
};

/**
 *
 */
class VariableManager
{
public:
    VariableManager() = default;

    VariableManager(const VariableManager&) = delete;

    /*
     *
     */
    CompoundVariable::VariableKeyValue CreateVariableFromLuaStack(lua_State*, int stackIndex, CompoundVariable& parent, ChildVariableKey key, std::string_view overrideVariableName = {}, VariableRegistration* = nullptr);

    VariableRegistration RegisterVariableRoot(CompoundVariablePtr);

    std::vector<dap::Variable> GetVariables(const dap::VariablesArguments&);

private:
    struct CompoundVariableEntry
    {
        unsigned id;
        unsigned parentId = 0;
        ChildVariableKey key;
        CompoundVariablePtr variable;
        std::optional<std::vector<CompoundVariable::VariableKeyValue>> children;

        CompoundVariableEntry(unsigned varId, unsigned parentVarId, ChildVariableKey varKey, CompoundVariablePtr varValue);

        CompoundVariableEntry(unsigned varId, CompoundVariablePtr varValue);
    };

    unsigned GetNextVariableId();

    CompoundVariableEntry* FindCompoundVariableEntry(unsigned refId);

    CompoundVariableEntry* FindCompoundVariableEntry(const CompoundVariable& variable);

    void RemoveVariableAndChildren(unsigned variableRef);

    std::list<CompoundVariableEntry> _compoundVariables;
    unsigned _nextVariableId = 0;

    friend class VariableRegistration;
};

}  // namespace my::lua
