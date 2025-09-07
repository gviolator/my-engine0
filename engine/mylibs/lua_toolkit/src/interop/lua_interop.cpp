// #my_engine_source_file
#include "lua_toolkit/lua_interop.h"

namespace my::lua
{
    namespace
    {
        static const ValueKeeperGuard* s_currentValueKeeperGuard = nullptr;
    } // namespace

    ValueKeeperGuard::ValueKeeperGuard(lua_State* inL, ValueKeepMode inKeepMode) :
        l(inL),
        keepMode(inKeepMode),
        parentGuard(std::exchange(s_currentValueKeeperGuard, this))
    {
        if (keepMode == ValueKeepMode::OnStack)
        {
            runtimeStackGuard.emplace();
        }
        else
        {
            luaStackGuard.emplace(l);
        }
    }

    ValueKeeperGuard::~ValueKeeperGuard()
    {
        [[maybe_unused]] const ValueKeeperGuard* const prev = std::exchange(s_currentValueKeeperGuard, parentGuard);
        MY_DEBUG_ASSERT(prev == this);
    }

    const ValueKeeperGuard* ValueKeeperGuard::current()
    {
        return s_currentValueKeeperGuard;
    }

}  // namespace my::lua