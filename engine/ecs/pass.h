#pragma once

#include "worldBase.h"
#include "access/mutex.h"

namespace fe::engine::ecs {
class WorldBase;

enum Priority : uint32_t { First = 0x00000000, High = 0x00001000, Mid = 0x00002000, Low = 0x00003000 };

class Pass {
 public:
  using CallType = std::function<void(WorldBase&)>;

  Pass(const stl::string& name = "") : m_name(name) {}
  ~Pass() = default;

  template <class R, class... Args>
  void init(R (&func)(Args...)) {
    m_call = [&func](WorldBase& world) mutable {
      func(world.getParam<Args>()...);
    };
    m_mutexs = detail::merge_mutex_vectors(detail::get_mutexes_for_type<Args>()...);

    m_preparers.clear();
    (detail::collect_preparers<Args>(m_preparers), ...);
  }

  bool m_isRepeat = true;
  uint32_t m_priority = 0;
  stl::unordered_set<stl::string> m_before;
  stl::unordered_set<stl::string> m_after;

 private:
  stl::string m_name;
  CallType m_call;
  stl::vector<Mutex> m_mutexs;

  stl::vector<std::function<void(Registry&)>> m_preparers;

  friend class World;
};
}  // namespace fe::engine::ecs