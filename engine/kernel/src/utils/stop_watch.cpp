// #my_engine_source_file
#include "my/utils/stop_watch.h"

namespace my
{
  Stopwatch::Stopwatch() :
      m_timePoint(std::chrono::system_clock::now())
  {
  }

  std::chrono::milliseconds Stopwatch::getTimePassed() const
  {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now() - m_timePoint);
  }

}  // namespace my
