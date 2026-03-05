#pragma once

#include "stage.h"
#include "worldBase.h"
#include "paramMutex.h"
#include "paramPrepare.h"
#include "paramAccess.h"
#include "meta.h"

namespace fe::engine::ecs {

enum Priority : uint32_t { First = 0x00000000, High = 0x00001000, Mid = 0x00002000, Low = 0x00003000 };

class Pass {
  using CallType = std::function<void()>;
  static inline uint32_t sId = 0;

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
    auto prev_hashes = stage::get_previous_hashes<T>();
    auto next_hashes = stage::get_next_hashes<T>();
    m_before_stage.insert(prev_hashes.begin(), prev_hashes.end());
    m_after_stage.insert(next_hashes.begin(), next_hashes.end());
    return *this;
  }

 private:
  // Parameter fetch helper: distinguishes pass-by-value vs pass-by-reference
  // Fix: Check original type T (preserving const) for Context Reader/Writer distinction
  template <typename T>
  static auto get_param(WorldBase& world, uint32_t passId) {
    // Check Resource types first (preserving const)
    if constexpr (is_resource_reader<T>::value) {
      return ParamAccess::get<T>(world.m_resource_manager);
    } else if constexpr (is_resource_writer<T>::value) {
      return ParamAccess::get<T>(world.m_resource_manager);
    } else {
      using RawT = meta::clean_t<T>;
      if constexpr (is_entity_command_buffer<RawT>::value) {
        // EntityCommandBuffer: return by reference
        return std::ref(ParamAccess::get<T>(world.m_entity_command_buffers, passId));
      } else if constexpr (is_entity_query<RawT>::value) {
        return ParamAccess::get<T>(world.m_registry);
      } else if constexpr (is_entity_creator<RawT>::value) {
        return ParamAccess::get<T>(world.m_registry);
      } else if constexpr (is_entity_destroyer<RawT>::value) {
        return ParamAccess::get<T>(world.m_registry);
      } else if constexpr (is_component_reader<RawT>::value) {
        return ParamAccess::get<T>(world.m_registry);
      } else if constexpr (is_component_writer<RawT>::value) {
        return ParamAccess::get<T>(world.m_registry);
      } else if constexpr (is_event_reader<RawT>::value) {
        return ParamAccess::get<T>(world.m_event_manager1);
      } else if constexpr (is_event_writer<RawT>::value) {
        return ParamAccess::get<T>(world.m_event_manager2);
      }
    }
  }

  template <typename Func, typename... Args>
  void init_impl(Func&& func, std::tuple<Args...>*) {
    m_mutexes = MutexCollector::merge(MutexCollector::get_for_type<Args>()...);
    m_preparers = PreparerCollector::get<Args...>();

    uint32_t passId = m_id;

    m_binder = [func = std::forward<Func>(func), passId](WorldBase& world) mutable -> CallType {
      // Use std::reference_wrapper to ensure correct reference passing
      return [func, params = std::make_tuple(get_param<Args>(world, passId)...)]() mutable {
        std::apply(func, params);
      };
    };
  }

  const uint32_t m_id = sId++;
  stl::string m_name;
  bool m_repeat = true;
  uint32_t m_priority = 0;

  stage::StageHash m_stage = 0;
  stl::unordered_set<stage::StageHash> m_before_stage;
  stl::unordered_set<stage::StageHash> m_after_stage;

  CallType m_execute;
  std::function<CallType(WorldBase&)> m_binder;

  stl::vector<Mutex> m_mutexes;
  stl::vector<std::function<void(WorldBase&)>> m_preparers;

  friend class World;
};

}  // namespace fe::engine::ecs
