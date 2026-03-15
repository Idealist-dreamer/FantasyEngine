#pragma once

#include "stage.h"
#include "paramAdapter.h"

namespace fe::engine {

enum Priority : uint32_t { First = 0x00000000, High = 0x00001000, Mid = 0x00002000, Low = 0x00003000 };

class Pass final {
  using CallType = std::function<void()>;
  static inline uint32_t s_next_id = 0;

 public:
  Pass(const stl::string& name, bool isRepeat = true, uint32_t priority = uint32_t(Priority::Low))
      : m_name(name), m_repeat(isRepeat), m_priority(priority) {}

  template <typename StageT, typename Func>
  static Pass create_start(const stl::string& name, Func&& func, uint32_t priority = uint32_t(Priority::Low)) {
    Pass pass(name, false, priority);
    pass.set_stage<StageT>();
    pass.init(std::forward<Func>(func));
    return pass;
  }

  template <typename StageT, typename Func>
  static Pass create_update(const stl::string& name, Func&& func, uint32_t priority = uint32_t(Priority::Low)) {
    Pass pass(name, true, priority);
    pass.set_stage<StageT>();
    pass.init(std::forward<Func>(func));
    return pass;
  }

  template <typename Func>
  void init(Func&& func) {
    using CleanFunc = std::remove_cvref_t<Func>;
    using ArgsTuple = typename meta::function_traits<CleanFunc>::args_tuple;
    init_impl<CleanFunc>(std::forward<Func>(func), static_cast<ArgsTuple*>(nullptr));
  }

  template <typename T>
  Pass& set_stage() {
    m_stage = stage::get_stage_hash<T>();
    return *this;
  }

  const uint32_t m_id = s_next_id++;
  stl::string m_name;
  bool m_repeat = true;
  uint32_t m_priority = 0;
  stage::StageHash m_stage = 0;

 private:
  template <typename Func, typename... Args>
  void init_impl(Func&& func, std::tuple<Args...>*) {
    m_mutexes = ParamOps::collect_mutexes<Args...>();
    m_preparers = ParamOps::collect_preparers<Args...>(m_id);

    uint32_t passId = m_id;
    m_binder = [func = std::forward<Func>(func), passId](SceneBase& scene) mutable -> CallType {
      return [func, params = ParamOps::fetch_all<Args...>(scene, passId)]() mutable {
        std::apply(func, params);
      };
    };
  }

  CallType m_execute;
  std::function<CallType(SceneBase&)> m_binder;

  stl::vector<Mutex> m_mutexes;
  stl::vector<std::function<void(SceneBase&)>> m_preparers;

  friend class Scene;
};

}  // namespace fe::engine
