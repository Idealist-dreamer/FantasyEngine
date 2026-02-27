#pragma once

#include "worldBase.h"
#include "access/mutex.h"

namespace fe::engine::ecs {
class WorldBase;

class Pass {
 public:
  using CallType = std::function<void(WorldBase&)>;

  Pass(const stl::string& name = "") : m_name(name) {}
  ~Pass() = default;

  template <class R, class... Args>
  void init(R (&func)(Args...)) {
    m_call = [&func](WorldBase& world) mutable {
      func(world.getSuper<Args>()...);
    };
    m_mutexs = detail::merge_mutex_vectors(detail::get_mutexes_for_type<Args>()...);
  }

  bool m_isRepeat = true;
  stl::unordered_set<stl::string> m_before;
  stl::unordered_set<stl::string> m_after;

 private:
  stl::string m_name;
  CallType m_call;
  stl::vector<Mutex> m_mutexs;

  friend class World;
};
}  // namespace fe::engine::ecs