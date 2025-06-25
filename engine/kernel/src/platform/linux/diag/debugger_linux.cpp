// #my_engine_source_file
#include <fstream>
#include <string>

#include "my/debug/debugger.h"

namespace my::debug
{
    bool isRunningUnderDebugger()
    {
        std::ifstream sf("/proc/self/status");
        std::string s;
        while (sf >> s)
        {
            if (s == "TracerPid:")
            {
                int pid;
                sf >> pid;
                return pid != 0;
            }
            std::getline(sf, s);
        }

        return false;
    }

}  // namespace my::debug
