// #my_engine_source_file

#pragma once

#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wshadow-uncaptured-local"
#endif  // __clang__

#include <memory>

#include "lua_toolkit/lua_headers.h"
#include "lua_toolkit/lua_toolkit_config.h"
#include "my/dispatch/class_descriptor.h"
#include "my/dispatch/dispatch.h"
#include "my/memory/mem_allocator.h"

namespace my::lua
{
    /**
     */
    MY_LUATOOLKIT_EXPORT
    Ptr<> makeValueFromLuaStack(lua_State* l, int index, IMemAllocator* = nullptr);

    // MY_LUATOOLKIT_EXPORT
    // Ptr<> makeRefValueFromLuaStack(lua_

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
        // auto& allocator = GetRtStackAllocator();
        IMemAllocator* const allocator = getSystemAllocatorPtr();

        return RuntimeValue::assign(
            makeValueRef(value, allocator),
            makeValueFromLuaStack(l, index, allocator));
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
