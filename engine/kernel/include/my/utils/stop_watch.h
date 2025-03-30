// #my_engine_source_file
#pragma once
#include <chrono>

#include "my/kernel/kernel_config.h"


namespace my
{
  class MY_KERNEL_EXPORT Stopwatch
  {
  public:
    using CountT = decltype(std::chrono::milliseconds{}.count());

    Stopwatch();

    std::chrono::milliseconds getTimePassed() const;

  private:
    const std::chrono::system_clock::time_point m_timePoint;
  };

}  // namespace my

// namespace std::chrono
// {
//   inline void PrintTo(milliseconds t, ostream* os)
//   {
//     *os << t.count() << "ms";
//   }

// }  // namespace std::chrono
