#pragma once

#include "worldBase.h"
#include "access/mutex.h"

namespace fe::engine::ecs {
class WorldBase;

enum struct Priority : uint32_t { First = 0x00000000, High = 0x00001000, Mid = 0x00002000, Low = 0x00003000 };

class Pass {
 public:
  using CallType = std::function<void(WorldBase&)>;

  Pass(const stl::string& name = "", bool isRepeat = true, uint32_t priority = uint32_t(Priority::Low))
      : m_name(name), m_isRepeat(isRepeat), m_priority(priority) {}
  ~Pass() = default;

  Pass& run_after(const stl::string& targetPass) {
    m_executeAfter.insert(targetPass);
    return *this;
  }

  Pass& run_before(const stl::string& targetPass) {
    m_executeBefore.insert(targetPass);
    return *this;
  }

  template <class R, class... Args>
  void init(R (&func)(Args...)) {
    m_call = [&func](WorldBase& world) mutable {
      func(world.get_param<Args>()...);
    };
    m_mutexs = detail::merge_mutex_vectors(detail::get_mutexes_for_type<Args>()...);

    m_preparers = detail::get_preparers<Args...>();
  }

  stl::string m_name;
  bool m_isRepeat = true;
  uint32_t m_priority = 0;
  stl::unordered_set<stl::string> m_executeBefore;
  stl::unordered_set<stl::string> m_executeAfter;

 private:
  CallType m_call;
  stl::vector<Mutex> m_mutexs;
  stl::vector<std::function<void(Registry&)>> m_preparers;

  friend class World;
};
}  // namespace fe::engine::ecs