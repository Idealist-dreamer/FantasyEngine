#pragma once
#include <chrono>

namespace fe::engine::utility {
class Timer {
 public:
  using Clock = std::chrono::steady_clock;
  using TimePoint = std::chrono::time_point<Clock>;

  Timer() : m_start(Clock::now()) {}

  FE_FINLINE void reset() { m_start = Clock::now(); }

  FE_FINLINE double getElapsedMs() const {
    auto end = Clock::now();
    auto duration = std::chrono::duration<double, std::milli>(end - m_start);
    return duration.count();
  }

  FE_FINLINE long long getElapsedNs() const {
    auto end = Clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - m_start).count();
  }

  FE_FINLINE float getElapsedSeconds() const { return static_cast<float>(getElapsedMs() / 1000.0); }

 private:
  TimePoint m_start;
};

}  // namespace fe::engine::utility