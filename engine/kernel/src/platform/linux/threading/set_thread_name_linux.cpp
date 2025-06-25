// #my_engine_source_file
#include <pthread.h>

#include "my/threading/set_thread_name.h"

namespace my::threading
{
    void setThisThreadName([[maybe_unused]] const std::string& name)
    {
        pthread_setname_np(pthread_self(), name.c_str());
    }
}  // namespace my::threading