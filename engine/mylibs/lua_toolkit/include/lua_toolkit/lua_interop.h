// #my_engine_source_file
#pragma once

#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wshadow-uncaptured-local"
#endif  // __clang__

#include <memory>
#include <optional>

#include "lua_toolkit/lua_headers.h"
#include "lua_toolkit/lua_toolkit_config.h"
#include "lua_toolkit/lua_utils.h"
#include "my/dispatch/class_descriptor.h"
#include "my/memory/runtime_stack.h"



namespace my::lua
{
    enum class ValueKeepMode
    {
        OnStack,
        AsReference
    };

    struct MY_LUATOOLKIT_EXPORT ValueKeeperGuard
    {
        lua_State* const l;
        const ValueKeepMode keepMode = ValueKeepMode::OnStack;
        //const int top;
        const ValueKeeperGuard* const parentGuard;
        std::optional<lua::StackGuard> luaStackGuard;
        std::optional<RuntimeStackGuard> runtimeStackGuard;

        ~ValueKeeperGuard();
        ValueKeeperGuard(lua_State* l, ValueKeepMode keepResult = ValueKeepMode::OnStack);
        ValueKeeperGuard() = delete;
        ValueKeeperGuard(const ValueKeeperGuard&) = delete;
        ValueKeeperGuard(ValueKeeperGuard&&) = delete;
        ValueKeeperGuard& operator=(const ValueKeeperGuard&) = delete;

        static const ValueKeeperGuard* current();
    };

    /**
     */
    MY_LUATOOLKIT_EXPORT
    std::tuple<Ptr<>, bool> makeValueFromLuaStack(lua_State* l, int index, std::optional<ValueKeepMode> overrideKeepMode = std::nullopt);

    MY_LUATOOLKIT_EXPORT
    Result<> pushRuntimeValue(lua_State* l, const RuntimeValuePtr& value);

    MY_LUATOOLKIT_EXPORT
    Result<> initializeClass(lua_State* l, ClassDescriptorPtr classDescriptor, bool keepMetatableOnStack);

    MY_LUATOOLKIT_EXPORT
    Result<> pushObject(lua_State* l, Ptr<> object, ClassDescriptorPtr classDescriptor);

    MY_LUATOOLKIT_EXPORT
    Result<> pushObject(lua_State* l, std::unique_ptr<IRttiObject> object, ClassDescriptorPtr classDescriptor);

    MY_LUATOOLKIT_EXPORT
    Result<> pushDispatch(lua_State* l, Ptr<> dispatch);

    MY_LUATOOLKIT_EXPORT
    Result<> populateTable(lua_State*, int index, const RuntimeValuePtr& value);

    /**
     */
    template <typename T>
    inline Result<> cast(lua_State* l, int index, T& value)
    {
        rtstack_scope;

        const auto [luaValue, _] = makeValueFromLuaStack(l, index, ValueKeepMode::OnStack);

        return RuntimeValue::assign(
            makeValueRef(value, getRtStackAllocatorPtr()),
            luaValue);
    }

    /**
     */
    template <typename T>
    inline Result<T> cast(lua_State* l, int index)
    {
        static_assert(std::is_default_constructible_v<T>, "Requires default constructor");

        T value{};
        CheckResult(cast(l, index, value))
        return value;
    }

}  // namespace my::lua

#ifdef __clang__
    #pragma clang diagnostic pop
#endif  // __clang__
