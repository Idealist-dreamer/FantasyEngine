#pragma once

#include <chrono>

namespace fe::engine::utility {
class Timer {
 public:
  using Clock = std::chrono::high_resolution_clock;

  Timer() { update(); }

  void update() { m_Start = Clock::now(); }

  long long getTimeNs() const { return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - m_Start).count(); }
  double getTimeMs() const { return getTimeNs() / 1000000.0; }

 private:
  std::chrono::time_point<Clock> m_Start;
};
}  // namespace fe::engine::utility