#pragma once

#include <chrono>

namespace fe::engine::utility {
class Timer {
 public:
  using Clock = std::chrono::high_resolution_clock;

  Timer() { Update(); }
  void Update() { m_Start = Clock::now(); }

  long long GetTimeNs() const { return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - m_Start).count(); }
  double GetTimeMs() const { return GetTimeNs() / 1000000.0; }

 private:
  std::chrono::time_point<Clock> m_Start;
};
}  // namespace fe::engine::utility