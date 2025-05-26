// #my_engine_source_file

#include "lua_internals.h"
#include "lua_toolkit/lua_interop.h"
#include "lua_toolkit/lua_utils.h"
// #include "my/memory/mem_allocator_std_wrapper.h"
#include "my/rtti/rtti_impl.h"
#include "my/rtti/weak_ptr.h"

namespace my::lua_detail
{
    struct PushValueGuard;

    // using ChildKeysArray = std::vector<lua::ChildVariableKey, MemAllocatorStdWrapper<lua::ChildVariableKey>>;
    using ChildKeysArray = std::vector<lua::ChildVariableKey>;

    /**
        Base class for any compound lua object: table, closure, stack.
        Responsible for pushing the 'child values' onto the lua stack via pushChild.
    */
    class MY_ABSTRACT_TYPE CompoundValue : public virtual IRefCounted
    {
        MY_INTERFACE(my::lua_detail::CompoundValue, IRefCounted)

    public:

        virtual lua_State* getLua() const = 0;

        virtual int pushChild(const lua::ChildVariableKey&) const = 0;

        inline Ptr<CompoundValue> getSelfPtr() const
        {
            auto mutableThis = const_cast<CompoundValue*>(this);
            return mutableThis;
        }
    };

    using CompoundValuePtr = Ptr<CompoundValue>;

    /**
        Stores a reference to the parent and the key by which the parent can find the given value.
        The object itself "does not know" how its value can be placed on the lua stack - parent + key is responsible for this.
    */
    class MY_ABSTRACT_TYPE ChildValue : public virtual IRefCounted
    {
        MY_INTERFACE(my::lua_detail::ChildValue, IRefCounted)
    public:
        ChildValue(CompoundValuePtr parent, lua::ChildVariableKey key) :
            m_parent(std::move(parent)),
            m_key(std::move(key))
        {
        }

        inline lua_State* getLua() const
        {
            MY_DEBUG_ASSERT(m_parent);
            return m_parent->getLua();
        }

        inline auto pushSelf() const
        {
            return m_parent->pushChild(m_key);
        }

    protected:
        const CompoundValuePtr m_parent;
        const lua::ChildVariableKey m_key;

        friend struct PushValueGuard;
    };

    class LuaFunctionDispatch final : public ChildValue,
                                      public IDispatch
    {
        MY_REFCOUNTED_CLASS(my::lua_detail::LuaFunctionDispatch, ChildValue, IDispatch)

    public:
        using ChildValue::ChildValue;

        Result<Ptr<>> invoke(std::string_view contract, std::string_view method, DispatchArguments args) override
        {
            lua_State* const l = getLua();

            guard_lstack(l);

            [[maybe_unused]]
            const auto index = pushSelf();
            for (size_t i = 0; i < args.size(); ++i)
            {
                lua::pushRuntimeValue(l, args[i]).ignore();
            }

            lua_call(l, args.size(), LUA_MULTRET);

            return nullptr;
        }

        ClassDescriptorPtr getClassDescriptor() const override
        {
            return nullptr;
        }
    };

    /*
        Base for implementing tables and arrays.
        An array in lua is the same table, but from the point of view of the Runtime view, access to the table and the array should be different.
    */
    class LuaTableValueBase : public ChildValue,
                              public CompoundValue
    {
        MY_INTERFACE(LuaTableValueBase, ChildValue, CompoundValue)
    public:
        LuaTableValueBase(CompoundValuePtr parent, lua::ChildVariableKey key, ChildKeysArray childKeys) :
            ChildValue(std::move(parent), std::move(key)),
            m_keys(std::move(childKeys))
        {
        }

        lua_State* getLua() const noexcept final
        {
            return static_cast<const ChildValue&>(*this).getLua();
        }

        int pushChild(const lua::ChildVariableKey& childKey) const final
        {
            lua_State* const l = getLua();
            const int selfIdx = m_parent->pushChild(m_key);
            MY_DEBUG_ASSERT(lua_type(l, selfIdx) == LUA_TTABLE);

            if (!childKey)
            {
                return selfIdx;
            }
            childKey.push(l);
            lua_gettable(l, selfIdx);

            return lua::getAbsoluteStackPos(l, -1);
        }

        Result<> setField(const lua::ChildVariableKey& childKey, const RuntimeValuePtr& value)
        {
            lua_State* const l = getLua();
            const int selfIdx = m_parent->pushChild(m_key);
            MY_DEBUG_ASSERT(lua_type(l, selfIdx) == LUA_TTABLE);

            childKey.push(l);

            lua_gettable(l, selfIdx);

            const auto fieldType = lua_type(l, -1);
            if (fieldType == LUA_TNIL || fieldType == LUA_TNUMBER || fieldType == LUA_TSTRING || fieldType == LUA_TBOOLEAN)
            {
                lua_pop(l, 1);
                childKey.push(l);
                CheckResult(lua::pushRuntimeValue(l, value))
                lua_settable(l, selfIdx);

                return {};
            }

            if (fieldType == LUA_TTABLE)
            {
                CheckResult(lua::populateTable(l, -1, value))
                return {};
            }

            return MakeError("Unexpected lua type");
        }

    protected:
        const ChildKeysArray m_keys;
    };

    /**
     */
    struct PushValueGuard : lua::StackGuard
    {
        const int index;

        PushValueGuard(const ChildValue& value, int expectedLuaType) :
            lua::StackGuard{value.getLua()},
            index(pushChildValue(value.getLua(), value, expectedLuaType))
        {
        }

        explicit operator bool() const
        {
            return index != lua::InvalidLuaIndex;
        }

        operator int() const
        {
            MY_DEBUG_ASSERT(index != lua::InvalidLuaIndex);
            return index;
        }

    private:
        static inline int pushChildValue(lua_State* l, const ChildValue& value, [[maybe_unused]] int expectedLuaType)
        {
            const int stackIndex = value.pushSelf();
            if (const int type = lua_type(l, stackIndex); type != expectedLuaType)
            {
                return lua::InvalidLuaIndex;
            }

            return stackIndex;
        }
    };

    RuntimeValuePtr createLuaRuntimeValue(CompoundValuePtr parent, lua::ChildVariableKey, IMemAllocator*);

    /**
     */

    class LuaStringValue final : public ChildValue,
                                 public RuntimeStringValue
    {
        MY_REFCOUNTED_CLASS(LuaStringValue, ChildValue, RuntimeStringValue)

    public:
        using ChildValue::ChildValue;

        bool isMutable() const override
        {
            return false;
        }

        Result<> setString(std::string_view) override
        {
            return ResultSuccess;
        }

        std::string getString() const override
        {
            if (const PushValueGuard index{*this, LUA_TSTRING}; index)
            {
                size_t len = 0;
                const char* const str = lua_tolstring(getLua(), index, &len);
                return std::string{str, len};
            }

            return {};
        }
    };

    /*
     */
    class MY_ABSTRACT_TYPE LuaIntegerValue : public RuntimeIntegerValue
    {
        MY_INTERFACE(LuaIntegerValue, RuntimeIntegerValue)

    public:
        bool isSigned() const final
        {
            return true;
        }

        size_t getBitsCount() const final
        {
            return sizeof(lua_Integer);
        }

        void setInt64(int64_t value) final
        {
            if (const PushValueGuard index(this->as<const ChildValue&>(), LUA_TNUMBER); index)
            {
                // const lua_Integer value = lua_tointeger(index.l, index);
            }
        }

        void setUint64(uint64_t value) final
        {
        }

        int64_t getInt64() const final
        {
            if (const PushValueGuard index(this->as<const ChildValue&>(), LUA_TNUMBER); index)
            {
                const lua_Integer value = lua_tointegerx(index.luaState, index, nullptr);
                return static_cast<int64_t>(value);
            }

            return 0;
        }

        uint64_t getUint64() const final
        {
            if (const PushValueGuard index(this->as<const ChildValue&>(), LUA_TNUMBER); index)
            {
                const lua_Integer value = lua_tointegerx(index.luaState, index, nullptr);
                return static_cast<uint64_t>(value);
            }

            return 0;
        }
    };

    /*
     */
    class MY_ABSTRACT_TYPE LuaFloatValue : public RuntimeFloatValue
    {
        MY_INTERFACE(LuaFloatValue, RuntimeFloatValue)
    public:
        size_t getBitsCount() const final
        {
            return sizeof(lua_Number);
        }

        void setDouble(double) final
        {
        }

        void setSingle(float) final
        {
        }

        double getDouble() const override
        {
            if (const PushValueGuard index(this->as<const ChildValue&>(), LUA_TNUMBER); index)
            {
                const lua_Number value = lua_tonumber(index.luaState, index);
                return static_cast<double>(value);
            }

            return 0.;
        }

        float getSingle() const override
        {
            if (const PushValueGuard index(this->as<const ChildValue&>(), LUA_TNUMBER); index)
            {
                const lua_Number value = lua_tonumber(index.luaState, index);
                if constexpr (sizeof(lua_Number) > sizeof(float))
                {
                    MY_DEBUG_ASSERT(value <= std::numeric_limits<float>::max(), "Numeric overflow");
                }
                return static_cast<float>(value);
            }

            return 0.f;
        }
    };

    /*
     */
    class LuaNumericValue final : public ChildValue,
                                  public LuaIntegerValue,
                                  public LuaFloatValue
    {
        MY_REFCOUNTED_CLASS(LuaNumericValue, ChildValue, LuaIntegerValue, LuaFloatValue)

    public:
        using ChildValue::ChildValue;

        bool isMutable() const override
        {
            return false;
        }
    };

    /**
     */
    class LuaBooleanValue final : public ChildValue,
                                  public RuntimeBooleanValue
    {
        MY_REFCOUNTED_CLASS(LuaBooleanValue, ChildValue, RuntimeBooleanValue)

    public:
        using ChildValue::ChildValue;

        bool isMutable() const override
        {
            return false;
        }

        void setBool(bool) override
        {
        }

        bool getBool() const override
        {
            if (const PushValueGuard index(*this, LUA_TBOOLEAN); index)
            {
                const bool value = (lua_toboolean(index.luaState, index) != 0);
                return value;
            }

            return false;
        }
    };
    /**
     */
    class LuaOptionalValue final : public ChildValue,
                                   public RuntimeOptionalValue
    {
        MY_REFCOUNTED_CLASS(LuaOptionalValue, ChildValue, RuntimeOptionalValue)

    public:
        using ChildValue::ChildValue;

        bool isMutable() const override
        {
            return false;
        }

        Result<> setValue(RuntimeValuePtr value) override
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
                                public RuntimeDictionary
    {
        MY_REFCOUNTED_CLASS(LuaTableValue, LuaTableValueBase, RuntimeDictionary)

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
            return m_keys[index].asString();
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

            return createLuaRuntimeValue(getSelfPtr(), *key, nullptr);
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
                                public RuntimeCollection
    {
        MY_REFCOUNTED_CLASS(LuaArrayValue, LuaTableValueBase, RuntimeCollection)

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
            return createLuaRuntimeValue(getSelfPtr(), *key, nullptr);
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

    /**
     */
    class LuaStackRootValue final : public CompoundValue
    {
        MY_REFCOUNTED_CLASS(LuaStackRootValue, CompoundValue)

    public:
        static Ptr<LuaStackRootValue> instance(lua_State* l)
        {
            using RootMap = std::unordered_map<lua_State*, my::WeakPtr<LuaStackRootValue>>;

            static thread_local RootMap roots;

            auto& rootRef = roots[l];
            auto root = rootRef.acquire();
            if (!root)
            {
                root = rtti::createInstance<LuaStackRootValue>(l);
                rootRef = root;
            }

            MY_DEBUG_ASSERT(root && root->m_lua == l);
            return root;
        }

        LuaStackRootValue(lua_State* l) :
            m_lua(l)
        {
            MY_DEBUG_ASSERT(m_lua);
        }

        lua_State* getLua() const override
        {
            return m_lua;
        }

        int pushChild(const lua::ChildVariableKey& key) const override
        {
            MY_DEBUG_ASSERT(key && key.isIndexed());
            return static_cast<int>(key);
        }

        RuntimeValuePtr wrapStackValue(int index, IMemAllocator* allocator)
        {
            return createLuaRuntimeValue(getSelfPtr(), lua::getAbsoluteStackPos(m_lua, index), allocator);
        }

    private:
        lua_State* const m_lua;
    };

    /**
     */
    class LuaGlobalRefRootValue final : public CompoundValue
    {
        MY_REFCOUNTED_CLASS(LuaGlobalRefRootValue, CompoundValue)

        static inline const char* GlobalRefsFieldName = "Nau__GlobalRefs";

    public:
        static Ptr<LuaGlobalRefRootValue> instance(lua_State* l)
        {
            using RootMap = std::unordered_map<lua_State*, my::WeakPtr<LuaGlobalRefRootValue>>;

            static thread_local RootMap roots;

            auto& rootRef = roots[l];
            auto root = rootRef.acquire();
            if (!root)
            {
                root = rtti::createInstance<LuaGlobalRefRootValue>(l);
                rootRef = root;
            }

            MY_DEBUG_ASSERT(root && root->m_lua == l);
            return root;
        }

        LuaGlobalRefRootValue(lua_State* l) :
            m_lua(l)
        {
            MY_DEBUG_ASSERT(m_lua);

#ifdef _DEBUG
            MY_DEBUG_ASSERT(lua_getglobal(m_lua, GlobalRefsFieldName) == LUA_TNIL);
            lua_pop(m_lua, 1);
#endif
            lua_createtable(m_lua, 0, 0);
            lua_setglobal(m_lua, GlobalRefsFieldName);
        }

        ~LuaGlobalRefRootValue()
        {
            lua_pushnil(m_lua);
            lua_setglobal(m_lua, GlobalRefsFieldName);
        }

        lua_State* getLua() const override
        {
            return m_lua;
        }

        int pushChild(const lua::ChildVariableKey& key) const override
        {
            MY_DEBUG_ASSERT(key && key.isIndexed());

            [[maybe_unused]]
            const auto t = lua_getglobal(m_lua, GlobalRefsFieldName);
            MY_DEBUG_ASSERT(t == LUA_TTABLE);

            lua_rawgeti(m_lua, -1, key);
            lua_remove(m_lua, -2);

            return lua_gettop(m_lua);
        }

        int keepReference(int stackIndex)
        {
            guard_lstack(m_lua);

            [[maybe_unused]]
            const auto t = lua_getglobal(m_lua, GlobalRefsFieldName);
            MY_DEBUG_ASSERT(t == LUA_TTABLE);

            lua_pushvalue(m_lua, stackIndex);
            return luaL_ref(m_lua, -2);
        }

        void releaseReference(int refId)
        {
        }

    private:
        lua_State* const m_lua;
    };

    /**
     */
    RuntimeValuePtr createLuaRuntimeValue(CompoundValuePtr parent, lua::ChildVariableKey key, IMemAllocator* allocator)
    {
        MY_DEBUG_ASSERT(parent);
        MY_DEBUG_ASSERT(key);

        if (!allocator)
        {
            allocator = getSystemAllocatorPtr();
        }

        auto* const l = parent->getLua();

        guard_lstack(l);

        const int index = parent->pushChild(key);
        const int type = lua_type(l, index);

        if (type == LUA_TNUMBER)
        {
            return rtti::createInstanceWithAllocator<LuaNumericValue>(allocator, std::move(parent), std::move(key));
        }
        else if (type == LUA_TBOOLEAN)
        {
            return rtti::createInstanceWithAllocator<LuaBooleanValue>(allocator, std::move(parent), std::move(key));
        }
        else if (type == LUA_TSTRING)
        {
            return rtti::createInstanceWithAllocator<LuaStringValue>(allocator, std::move(parent), std::move(key));
        }
        else if (type == LUA_TTABLE)
        {
            const lua::TableEnumerator fields{l, parent->pushChild(key)};
            const size_t keysCount = std::distance(fields.begin(), fields.end());
            bool isArray = keysCount > 0;  // empty table -> dict

            ChildKeysArray childKeys;

            if (keysCount > 0)
            {
                childKeys.reserve(keysCount);
                int lastChildIndex = lua::InvalidLuaIndex;

                for (auto [childKeyIndex, _] : fields)
                {
                    lua::ChildVariableKey& childKey = childKeys.emplace_back(lua::ChildVariableKey::makeFromStack(l, childKeyIndex));

                    if (isArray)
                    {
                        // array if all keys are integers + each key must be greater than the previous one (monotonicity is not taken into account).
                        if (!childKey.isIndexed() || (lastChildIndex != lua::InvalidLuaIndex && childKey < lastChildIndex))
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
                return rtti::createInstanceWithAllocator<LuaArrayValue, RuntimeValue>(std::move(allocator), std::move(parent), std::move(key), std::move(childKeys));
            }

            return rtti::createInstanceWithAllocator<LuaTableValue, RuntimeValue>(std::move(allocator), std::move(parent), std::move(key), std::move(childKeys));
        }
        else if (type == LUA_TNIL)
        {
            return rtti::createInstanceWithAllocator<LuaOptionalValue>(std::move(allocator), std::move(parent), std::move(key));
        }

        MY_DEBUG_FATAL_FAILURE("Unknown lua type");

        return nullptr;
    }

    // Ptr<> createDispatchForLuaFunc(lua_State* l, int index)
    // {

    // }
}  // namespace my::lua_detail

namespace my::lua
{
    Ptr<> makeValueFromLuaStack(lua_State* l, int index, IMemAllocator* allocator)
    {
        if (lua_type(l, index) == LUA_TFUNCTION)
        {
            auto globalsRoot = lua_detail::LuaGlobalRefRootValue::instance(l);

            const int refId = globalsRoot->keepReference(index);
            return rtti::createInstance<lua_detail::LuaFunctionDispatch>(std::move(globalsRoot), refId);
        }

        return lua_detail::LuaStackRootValue::instance(l)->wrapStackValue(index, allocator);
    }

    Result<> pushRuntimeValue(lua_State* l, const RuntimeValuePtr& value)
    {
        MY_DEBUG_ASSERT(l);
        MY_DEBUG_ASSERT(value);

        if (RuntimeOptionalValue* const optValue = value->as<RuntimeOptionalValue*>())
        {
            if (!optValue->hasValue())
            {
                lua_pushnil(l);
                return {};
            }

            return pushRuntimeValue(l, optValue->getValue());
        }
        else if (const RuntimeStringValue* const strValue = value->as<const RuntimeStringValue*>())
        {
            const std::string str = strValue->getString();
            lua_pushlstring(l, str.data(), str.size());
        }
        else if (const RuntimeBooleanValue* const boolValue = value->as<const RuntimeBooleanValue*>())
        {
            const int b = boolValue->getBool() ? 1 : 0;
            lua_pushboolean(l, b);
        }
        else if (const RuntimeIntegerValue* const intValue = value->as<const RuntimeIntegerValue*>())
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
        else if (const RuntimeFloatValue* const floatValue = value->as<const RuntimeFloatValue*>())
        {
            const double fValue = floatValue->getDouble();
            lua_pushnumber(l, static_cast<lua_Number>(fValue));
        }
        else if (RuntimeReadonlyCollection* const collection = value->as<RuntimeReadonlyCollection*>())
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
        else if (RuntimeReadonlyDictionary* const dict = value->as<RuntimeReadonlyDictionary*>())
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

        if (auto* const dict = value->as<RuntimeReadonlyDictionary*>())
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
        else if (const auto collection = value->as<const RuntimeReadonlyCollection*>())
        {
        }
        else
        {
            return MakeError("Value must be collection or dictionary");
        }

        return {};
    }

}  // namespace my::lua
