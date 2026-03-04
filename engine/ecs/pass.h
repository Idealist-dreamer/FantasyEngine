#pragma once

#include "stage.h"
#include "worldBase.h"

#include "paramMutex.h"
#include "paramDetail.h"

namespace fe::engine::ecs {
class WorldBase;

enum Priority : uint32_t { First = 0x00000000, High = 0x00001000, Mid = 0x00002000, Low = 0x00003000 };

class Pass {
 public:
  using CallType = std::function<void()>;

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

    init_impl<CleanFunc>(std::forward<Func>(func), (ArgsTuple*)nullptr);
  }

  template <typename T>
  Pass& set_stage() {
    m_stage = get_stage_hash<T>();
    m_before_stage.insert(get_previous_hashes<T>());
    m_after_stage.insert(get_next_hashes<T>());
    return *this;
  }

 private:
  template <typename Func, typename... Args>
  void init_impl(Func&& func, std::tuple<Args...>*) {
    m_mutexes = Detail::merge_mutex_vectors(Detail::get_mutexes_for_type<Args>()...);
    m_preparers = Detail::get_preparers<Args...>();

    uint32_t passId = m_id;

    m_binder = [func = std::forward<Func>(func), passId](WorldBase& world) mutable -> CallType {
      return [func, params = std::make_tuple(world.get_param<Args>(passId)...)]() mutable {
        std::apply(func, params);
      };
    };
  }

  static inline uint32_t sId = 0;
  const uint32_t m_id = sId++;

  stl::string m_name;
  bool m_repeat = true;
  uint32_t m_priority = 0;

  StageHash m_stage = 0;
  stl::unordered_set<StageHash> m_before_stage;
  stl::unordered_set<StageHash> m_after_stage;

  CallType m_execute;
  std::function<CallType(WorldBase&)> m_binder;

  stl::vector<Mutex> m_mutexes;
  stl::vector<std::function<void(WorldBase&)>> m_preparers;

  friend class World;
};
}  // namespace fe::engine::ecs