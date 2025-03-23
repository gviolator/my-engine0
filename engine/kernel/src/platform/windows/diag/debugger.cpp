// #my_engine_source_header

#include "my/debug/debugger.h"

#include <crtdbg.h>
#include <debugapi.h>

namespace my::debug
{
    bool isRunningUnderDebugger()
    {
        return ::IsDebuggerPresent() == TRUE;
    }

}  // namespace my::debug
