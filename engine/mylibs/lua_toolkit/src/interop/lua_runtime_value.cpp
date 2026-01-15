// #my_engine_source_file
#include "lua_runtime_value.h"

#include "my/dispatch/closure_value.h"
#include "my/memory/allocator.h"
#include "value_roots.h"

namespace my::lua_detail
{
    namespace
    {
        using ChildKeysArray = std::pmr::vector<lua::ChildVariableKey>;

        inline bool isReferenceType(int type)
        {
            return type == LUA_TFUNCTION || type == LUA_TTABLE;
        }

        inline bool isValueType(int type)
        {
            return type == LUA_TNIL || type == LUA_TNUMBER || type == LUA_TSTRING || type == LUA_TBOOLEAN;
        }
    }  // namespace

    ReferenceValue::ReferenceValue(LuaRootPtr root, lua::ChildVariableKey key) :
        m_root(std::move(root)),
        m_key(std::move(key))
    {
        MY_DEBUG_FATAL(m_root);
        MY_DEBUG_FATAL(m_key);
    }

    ReferenceValue::~ReferenceValue()
    {
        m_root->unref(m_key);
    }

    lua_State* ReferenceValue::getLua() const
    {
        return m_root->getLua();
    }

    int ReferenceValue::pushSelf() const
    {
        return m_root->push(m_key);
    }

    /**
     */

    class LuaClosureValue final : public ReferenceValue,
                                  public ClosureValue

    {
        MY_REFCOUNTED_CLASS(my::lua_detail::LuaClosureValue, ReferenceValue, ClosureValue)

    public:
        using ReferenceValue::ReferenceValue;

        Result<Ptr<>> invoke(DispatchArguments args) override
        {
            using namespace lua;

            lua_State* const l = getLua();

            // const ValueKeepMode keepMode = EXPR_Block
            // {
            //     const ValueKeeperGuard* const keepGuard = ValueKeeperGuard::current();
            //     return keepGuard ? keepGuard->keepMode : ValueKeepMode::AsReference;
            // };

            const int top = lua_gettop(l);
            [[maybe_unused]] const auto index = pushSelf();

            for (Ptr<>& arg : args)
            {
                CheckResult(lua::pushRuntimeValue(l, arg));
            }

            Lua_CheckErr(l, lua_pcall(l, args.size(), LUA_MULTRET, 0));

            const int retCount = lua_gettop(l) - top;
            MY_DEBUG_ASSERT(retCount >= 0);

            if (retCount == 0)
            {
                return nullptr;
            }
            MY_DEBUG_ASSERT(retCount == 1, "Multiple function results not supported");

            auto [value, keepStackValue] = makeValueFromLuaStack(l, -1);
            if (!keepStackValue)
            {
                lua_settop(l, top);
            }

            return {std::move(value)};
        }
    };

    /*
        Base for implementing tables and arrays.
        An array in lua is the same table, but from the point of view of the Runtime view, access to the table and the array should be different.
    */
    class LuaTableValueBase : public ReferenceValue
    {
        MY_INTERFACE(LuaTableValueBase, ReferenceValue)
    public:
        LuaTableValueBase(LuaRootPtr root, lua::ChildVariableKey key, ChildKeysArray childKeys) :
            ReferenceValue(std::move(root), std::move(key)),
            m_keys(std::move(childKeys))
        {
        }

        int pushChild(const lua::ChildVariableKey& childKey) const
        {
            lua_State* const l = getLua();
            const int tblIndex = pushSelf();
            MY_DEBUG_ASSERT(lua_type(l, tblIndex) == LUA_TTABLE);

            if (!childKey)
            {
                return tblIndex;
            }

            childKey.Push(l);
            [[maybe_unused]] const int t = lua_gettable(l, tblIndex);
            if (t == LUA_TNIL)
            {
            }

            lua_remove(l, -2);
            return lua::getAbsoluteStackPos(l, -1);
        }

        Result<> setField(const lua::ChildVariableKey& childKey, const RuntimeValuePtr& value)
        {
            lua_State* const l = getLua();
            const int selfIdx = pushSelf();
            MY_DEBUG_ASSERT(lua_type(l, selfIdx) == LUA_TTABLE);

            childKey.Push(l);
            lua_gettable(l, selfIdx);

            const auto fieldType = lua_type(l, -1);
            if (isValueType(fieldType))
            {
                lua_pop(l, 1);
                childKey.Push(l);
                CheckResult(lua::pushRuntimeValue(l, value))
                lua_settable(l, selfIdx);

                if (fieldType == LUA_TNIL && !hasKey(childKey))
                {
                    m_keys.push_back(childKey);
                }

                return ResultSuccess;
            }

            if (fieldType == LUA_TTABLE)
            {
                CheckResult(lua::populateTable(l, -1, value))
                return ResultSuccess;
            }

            return MakeError("Unexpected lua type");
        }

    protected:
        bool hasKey(const lua::ChildVariableKey& key) const
        {
            return std::find_if(m_keys.begin(), m_keys.end(), [&key](const lua::ChildVariableKey& childKey)
            {
                return childKey == key;
            }) != m_keys.end();
        }

        RuntimeValuePtr makeChildValue(const lua::ChildVariableKey& key) const
        {
            lua_State* const l = getLua();
            const int idx = pushChild(key);
            auto [childValue, keepStackValue] = lua::makeValueFromLuaStack(l, idx);
            if (!keepStackValue)
            {
                lua_pop(l, 1);
            }

            return childValue;
        }

        ChildKeysArray m_keys;
    };

    /**
     */
    class LuaStringValue final : public StringValue
    {
        MY_REFCOUNTED_CLASS(LuaStringValue, StringValue)

    public:
        LuaStringValue(lua_State* l, int idx, IAllocator& allocator) :
            m_value(allocator.getMemoryResource())
        {
            MY_DEBUG_FATAL(l);
            MY_DEBUG_ASSERT(lua_type(l, idx) == LUA_TSTRING);
            size_t len = 0;
            const char* const str = lua_tolstring(l, idx, &len);
            m_value.assign(str, len);
        }

        bool isMutable() const override
        {
            return false;
        }

        Result<> setString(std::string_view) override
        {
            MY_DEBUG_FAILURE("direct set primitive lua value is not supported");
            return MakeError("Direct set primitive value is not supported");
        }

        std::string getString() const override
        {
            return std::string{m_value.data(), m_value.size()};
        }

        std::pmr::string m_value;
    };

    class LuaNumericValue final : public IntegerValue,
                                  public FloatValue
    {
        MY_REFCOUNTED_CLASS(LuaNumericValue, IntegerValue, FloatValue)

    public:
        LuaNumericValue(lua_State* l, int idx)
        {
            MY_DEBUG_FATAL(l);
            MY_DEBUG_ASSERT(lua_type(l, idx) == LUA_TNUMBER);

            [[maybe_unused]] int convOk = true;
            m_value = lua_tonumberx(l, idx, &convOk);
            MY_DEBUG_ASSERT(convOk);
        }

        bool isMutable() const override
        {
            return false;
        }

        bool isSigned() const final
        {
            return true;
        }

        size_t getBitsCount() const final
        {
            return sizeof(lua_Number);
        }

        void setInt64(int64_t) final
        {
            MY_DEBUG_FAILURE("direct set primitive lua value is not supported");
        }

        void setUint64(uint64_t) final
        {
            MY_DEBUG_FAILURE("direct set primitive lua value is not supported");
        }

        int64_t getInt64() const final
        {
            return static_cast<int64_t>(m_value);
        }

        uint64_t getUint64() const final
        {
            return static_cast<uint64_t>(m_value);
        }

        void setDouble(double) final
        {
            MY_DEBUG_FAILURE("direct set primitive lua value is not supported");
        }

        void setSingle(float) final
        {
            MY_DEBUG_FAILURE("direct set primitive lua value is not supported");
        }

        double getDouble() const override
        {
            return static_cast<double>(m_value);
        }

        float getSingle() const override
        {
#if MY_DEBUG_ASSERT_ENABLED
            if constexpr (sizeof(lua_Number) > sizeof(float))
            {
                MY_DEBUG_ASSERT(m_value <= std::numeric_limits<float>::max(), "Numeric overflow");
            }
#endif
            return static_cast<float>(m_value);
        }

    private:
        lua_Number m_value = 0;
    };

    /**
     */
    class LuaBooleanValue final : public BooleanValue
    {
        MY_REFCOUNTED_CLASS(LuaBooleanValue, BooleanValue)

    public:
        LuaBooleanValue(lua_State* l, int idx)
        {
            m_value = lua_toboolean(l, idx) != 0;
        }

        bool isMutable() const override
        {
            return false;
        }

        void setBool(bool) override
        {
            MY_DEBUG_FAILURE("direct set primitive lua value is not supported");
        }

        bool getBool() const override
        {
            return m_value;
        }

    private:
        bool m_value = false;
    };

    /**
     */
    class LuaNilValue final : public OptionalValue
    {
        MY_REFCOUNTED_CLASS(LuaNilValue, OptionalValue)

    public:
        LuaNilValue([[maybe_unused]] lua_State* l, [[maybe_unused]] int idx)
        {
            MY_DEBUG_ASSERT(lua_type(l, idx) == LUA_TNIL);
        }

        bool isMutable() const override
        {
            return false;
        }

        Result<> setValue(RuntimeValuePtr) override
        {
            return {};
        }

        bool hasValue() const override
        {
            return false;
        }

        RuntimeValuePtr getValue() override
        {
            return nullptr;
        }
    };

    class LuaTableValue final : public LuaTableValueBase,
                                public Dictionary
    {
        MY_REFCOUNTED_CLASS(LuaTableValue, LuaTableValueBase, Dictionary)

    public:
        using LuaTableValueBase::LuaTableValueBase;

        bool isMutable() const override
        {
            return true;
        }

        size_t getSize() const override
        {
            return m_keys.size();
        }

        std::string_view getKey(size_t index) const override
        {
            MY_DEBUG_ASSERT(index < getSize());
            return m_keys[index].AsString();
        }

        RuntimeValuePtr getValue(std::string_view keyName) override
        {
            auto key = std::find_if(m_keys.begin(), m_keys.end(), [keyName](const lua::ChildVariableKey& childKey)
            {
                return childKey == keyName;
            });

            if (key == m_keys.end())
            {
                return nullptr;
            }

            return makeChildValue(*key);
        }

        bool containsKey(std::string_view keyName) const override
        {
            auto key = std::find_if(m_keys.begin(), m_keys.end(), [keyName](const lua::ChildVariableKey& childKey)
            {
                return childKey == keyName;
            });

            return key != m_keys.end();
        }

        void clear() override
        {
        }

        Result<> setValue(std::string_view name, const RuntimeValuePtr& value) override
        {
            return setField(lua::ChildVariableKey{name}, value);
        }

        RuntimeValuePtr erase(std::string_view name) override
        {
            return nullptr;
        }
    };

    /*
     */
    class LuaArrayValue final : public LuaTableValueBase,
                                public Collection
    {
        MY_REFCOUNTED_CLASS(LuaArrayValue, LuaTableValueBase, Collection)

    public:
        using LuaTableValueBase::LuaTableValueBase;

        bool isMutable() const override
        {
            return false;
        }

        size_t getSize() const override
        {
            return m_keys.size();
        }

        RuntimeValuePtr getAt(size_t index) override
        {
            MY_DEBUG_ASSERT(index < getSize());

            auto key = m_keys.begin();
            std::advance(key, index);

            return makeChildValue(*key);
        }

        Result<> setAt([[maybe_unused]] size_t index, [[maybe_unused]] const RuntimeValuePtr& value) override
        {
            MY_FAILURE("LuaArrayValue::setAt not implemented");
            return ResultSuccess;
        }

        void clear() override
        {
        }

        void reserve(size_t) override
        {
        }

        Result<> append(const RuntimeValuePtr&) override
        {
            return MakeError("Modification of the lua collection is not implemented");
        }
    };

    // /**
    //  */
    // Ptr<> createLuaRuntimeValue(LuaRootPtr root, int index, lua::ChildVariableKey&& childKey, IAllocator* allocator)
    // {
    // }

}  // namespace my::lua_detail

namespace my::lua
{
    std::tuple<Ptr<>, bool> makeValueFromLuaStack(lua_State* l, int index, std::optional<ValueKeepMode> overrideKeepMode)
    {
        using namespace my::lua_detail;

        MY_DEBUG_FATAL(l != nullptr);
        // TODO: may be need to manage ValueKeeperGuard per state ?
        MY_DEBUG_FATAL(!ValueKeeperGuard::current() || ValueKeeperGuard::current()->l == l);

        const ValueKeepMode keepMode = EXPR_Block
        {
            if (overrideKeepMode)
            {
                return *overrideKeepMode;
            }

            const ValueKeeperGuard* const keepGuard = ValueKeeperGuard::current();
            return keepGuard ? keepGuard->keepMode : ValueKeepMode::AsReference;
        };

        // value types (number, string, bool) must not be stored as referenced
        // and always returning as copies, so lua root does not need for it.
        // (because it will be copied into native type)

        const int type = lua_type(l, index);
        const bool isReference = lua_detail::isReferenceType(type);
        const bool keepValueOnStack = keepMode == ValueKeepMode::OnStack;
        IAllocator* const allocator = keepValueOnStack ? getRtStackAllocatorPtr() : getDefaultAllocatorPtr();
        LuaRootPtr root = isReference ? (keepValueOnStack ? LuaStackRoot::instance(l) : LuaGlobalRefRoot::instance(l)) : nullptr;

        ChildVariableKey childKey = root ? root->ref(index) : nullptr;
        MY_DEBUG_ASSERT(static_cast<bool>(childKey) != isValueType(type));
        //
        // guard_lstack(l);
        assert_lstack_unchanged(l);

        RuntimeValuePtr value;

        if (type == LUA_TSTRING)
        {
            value = rtti::createInstanceWithAllocator<LuaStringValue>(allocator, l, index, *allocator);
        }
        else if (type == LUA_TNUMBER)
        {
            value = rtti::createInstanceWithAllocator<LuaNumericValue>(allocator, l, index);
        }
        else if (type == LUA_TBOOLEAN)
        {
            value = rtti::createInstanceWithAllocator<LuaBooleanValue>(allocator, l, index);
        }
        else if (type == LUA_TNIL)
        {
            value = rtti::createInstanceWithAllocator<LuaNilValue>(allocator, l, index);
        }
        else if (type == LUA_TFUNCTION)
        {
            value = rtti::createInstanceWithAllocator<lua_detail::LuaClosureValue>(allocator, std::move(root), std::move(childKey));
        }
        else if (type == LUA_TTABLE)
        {
            const lua::TableEnumerator fields{l, index};
            const size_t keysCount = std::distance(fields.begin(), fields.end());
            bool isArray = keysCount > 0;  // empty table -> dict

            ChildKeysArray childKeys{allocator->getMemoryResource()};

            if (keysCount > 0)
            {
                childKeys.reserve(keysCount);
                int lastChildIndex = lua::InvalidLuaIndex;

                for (auto [childKeyIndex, _] : fields)
                {
                    lua::ChildVariableKey& childKey = childKeys.emplace_back(lua::ChildVariableKey::MakeFromStack(l, childKeyIndex));

                    if (isArray)
                    {
                        // array if all keys are integers + each key must be greater than the previous one (monotonicity is not taken into account).
                        if (!childKey.IsIndexed() || (lastChildIndex != lua::InvalidLuaIndex && childKey < lastChildIndex))
                        {
                            isArray = false;
                        }
                        else
                        {
                            lastChildIndex = childKey;
                        }
                    }
                }
            }
            if (isArray)
            {
                value = rtti::createInstanceWithAllocator<LuaArrayValue, RuntimeValue>(allocator, std::move(root), std::move(childKey), std::move(childKeys));
            }
            else
            {
                value = rtti::createInstanceWithAllocator<LuaTableValue, RuntimeValue>(allocator, std::move(root), std::move(childKey), std::move(childKeys));
            }
        }

        MY_DEBUG_FATAL(value, "Do not known how to represent lua type:({})", type);
        const bool needKeepStackValue = keepValueOnStack && isReference;
        return {std::move(value), needKeepStackValue};
    }

    Result<> pushRuntimeValue(lua_State* l, const RuntimeValuePtr& value)
    {
        MY_DEBUG_ASSERT(l);
        MY_DEBUG_ASSERT(value);

        if (OptionalValue* const optValue = value->as<OptionalValue*>())
        {
            if (!optValue->hasValue())
            {
                lua_pushnil(l);
                return {};
            }

            return pushRuntimeValue(l, optValue->getValue());
        }
        else if (const StringValue* const strValue = value->as<const StringValue*>())
        {
            const std::string str = strValue->getString();
            lua_pushlstring(l, str.data(), str.size());
        }
        else if (const BooleanValue* const boolValue = value->as<const BooleanValue*>())
        {
            const int b = boolValue->getBool() ? 1 : 0;
            lua_pushboolean(l, b);
        }
        else if (const IntegerValue* const intValue = value->as<const IntegerValue*>())
        {
            const auto i = EXPR_Block->lua_Integer
            {
                if (intValue->isSigned())
                {
                    return static_cast<lua_Integer>(intValue->getInt64());
                }

                return static_cast<lua_Integer>(intValue->getUint64());
            };

            lua_pushinteger(l, i);
        }
        else if (const FloatValue* const floatValue = value->as<const FloatValue*>())
        {
            const double fValue = floatValue->getDouble();
            lua_pushnumber(l, static_cast<lua_Number>(fValue));
        }
        else if (ReadonlyCollection* const collection = value->as<ReadonlyCollection*>())
        {
            const size_t size = collection->getSize();
            lua_createtable(l, static_cast<int>(size), 0);
            const int tableIndex = lua_gettop(l);

            for (size_t i = 0; i < size; ++i)
            {
                const RuntimeValuePtr element = (*collection)[i];
                lua_pushinteger(l, static_cast<lua_Integer>(i + 1));  // first element's index  = 1
                CheckResult(pushRuntimeValue(l, element));

                lua_rawset(l, tableIndex);
            }
        }
        else if (ReadonlyDictionary* const dict = value->as<ReadonlyDictionary*>())
        {
            const size_t size = dict->getSize();
            lua_createtable(l, 0, static_cast<int>(size));
            const int tableIndex = lua_gettop(l);

            for (size_t i = 0; i < size; ++i)
            {
                const auto [fieldKey, fieldValue] = (*dict)[i];
                lua_pushlstring(l, fieldKey.data(), fieldKey.size());
                CheckResult(pushRuntimeValue(l, fieldValue));
                lua_rawset(l, tableIndex);
            }
        }
        else
        {
            return MakeError("Dont known how to push value");
        }

        return {};
    }

    Result<> populateTable(lua_State* l, int index, const my::RuntimeValuePtr& value)
    {
        const int tablePos = getAbsoluteStackPos(l, index);

        MY_DEBUG_ASSERT(value);
        MY_DEBUG_ASSERT(lua_type(l, tablePos) == LUA_TTABLE);

        if (auto* const dict = value->as<ReadonlyDictionary*>())
        {
            for (size_t i = 0, size = dict->getSize(); i < size; ++i)
            {
                const auto [fieldKey, fieldValue] = (*dict)[i];
                lua_pushlstring(l, fieldKey.data(), fieldKey.size());
                lua_gettable(l, tablePos);

                const auto fieldType = lua_type(l, -1);
                if (fieldType == LUA_TTABLE)
                {
                    CheckResult(populateTable(l, -1, fieldValue));
                }
                else
                {
                    lua_pop(l, 1);
                    lua_pushlstring(l, fieldKey.data(), fieldKey.size());
                    CheckResult(pushRuntimeValue(l, fieldValue));
                    lua_settable(l, tablePos);
                }
            }
        }
        else if (const auto collection = value->as<const ReadonlyCollection*>())
        {
        }
        else
        {
            return MakeError("Value must be collection or dictionary");
        }

        return {};
    }

}  // namespace my::lua
