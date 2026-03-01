#pragma once
#include <chrono>

namespace fe::engine::utility {
class Timer {
 public:
  using Clock = std::chrono::steady_clock;
  using TimePoint = std::chrono::time_point<Clock>;

  Timer() : m_start(Clock::now()) {}

  FE_FINLINE void reset() { m_start = Clock::now(); }

  FE_FINLINE double get_elapsed_ms() const {
    auto end = Clock::now();
    auto duration = std::chrono::duration<double, std::milli>(end - m_start);
    return duration.count();
  }

  FE_FINLINE long long get_elapsed_ns() const {
    auto end = Clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - m_start).count();
  }

  FE_FINLINE float get_elapsed_seconds() const { return static_cast<float>(get_elapsed_ms() / 1000.0); }

 private:
  TimePoint m_start;
};

}  // namespace fe::engine::utility