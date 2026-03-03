#pragma once

#include "worldBase.h"

#include "paramMutex.h"
#include "paramDetail.h"

namespace fe::engine::ecs {
class WorldBase;

enum struct Priority : uint32_t { First = 0x00000000, High = 0x00001000, Mid = 0x00002000, Low = 0x00003000 };

class Pass {
 public:
  using CallType = std::function<void(WorldBase&)>;

  Pass(const stl::string& name, bool isRepeat = true, uint32_t priority = uint32_t(Priority::Low))
      : m_name(name), m_repeat(isRepeat), m_priority(priority) {}
  ~Pass() = default;

  template <typename Func>
  static Pass create_start(const stl::string& name, Func&& func, uint32_t priority = uint32_t(Priority::Low)) {
    Pass pass(name, false, priority);
    pass.init(std::forward<Func>(func));
    return pass;
  }

  template <typename Func>
  static Pass create_update(const stl::string& name, Func&& func, uint32_t priority = uint32_t(Priority::Low)) {
    Pass pass(name, true, priority);
    pass.init(std::forward<Func>(func));
    return pass;
  }

  template <typename Func>
  void init(Func&& func) {
    using CleanFunc = std::remove_cvref_t<Func>;
    using ArgsTuple = typename function_traits<CleanFunc>::args_tuple;
    init_impl(std::forward<Func>(func), ArgsTuple{});
  }

  template <typename T>
  Pass& set_stage() {
    m_stage = typeid(T).name();
    return *this;
  }

  template <typename T>
  Pass& set_after_stage() {
    m_after_stage.insert(typeid(T).name());
    return *this;
  }

  template <typename T>
  Pass& set_before_stage() {
    m_before_stage.insert(typeid(T).name());
    return *this;
  }

 private:
  template <typename Func, typename... Args>
  void init_impl(Func&& func, std::tuple<Args...>) {
    m_mutexes = Detail::merge_mutex_vectors(Detail::get_mutexes_for_type<Args>()...);

    m_preparers = Detail::get_preparers<Args...>();

    m_call = [func = std::forward<Func>(func)](WorldBase& world) mutable {
      func(world.get_param<Args>()...);
    };
  }

  stl::string m_name;
  bool m_repeat = true;
  uint32_t m_priority = 0;

  stl::string m_stage;
  stl::unordered_set<stl::string> m_before_stage;
  stl::unordered_set<stl::string> m_after_stage;

  CallType m_call;
  stl::vector<Mutex> m_mutexes;
  stl::vector<std::function<void(WorldBase&)>> m_preparers;

  friend class World;
};
}  // namespace fe::engine::ecs