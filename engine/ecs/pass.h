#pragma once

#include "meta.h"
#include "stage.h"
#include "worldBase.h"
#include "paramMutex.h"
#include "paramPrepare.h"
#include "paramAccess.h"

namespace fe::engine {

enum Priority : uint32_t { First = 0x00000000, High = 0x00001000, Mid = 0x00002000, Low = 0x00003000 };

class Pass {
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
  // Parameter fetch helper: distinguishes pass-by-value vs pass-by-reference
  // Fix: Check original type T (preserving const) for Context Reader/Writer distinction
  template <typename T>
  static auto get_param(Visitor<WorldBase>& visitor, uint32_t passId) {
    // Check context types first (preserving const)
    if constexpr (is_context_reader<T>::value) {
      return ParamAccess::get<T>(visitor->m_context_manager);
    } else if constexpr (is_context_writer<T>::value) {
      return ParamAccess::get<T>(visitor->m_context_manager);
    } else {
      using RawT = meta::clean_t<T>;
      if constexpr (is_entity_command_buffer<RawT>::value) {
        // EntityCommandBuffer: return by reference
        return std::ref(ParamAccess::get<T>(visitor->m_entity_command_buffers, passId));
      } else if constexpr (is_entity_query<RawT>::value) {
        return ParamAccess::get<T>(visitor->m_registry);
      } else if constexpr (is_entity_creator<RawT>::value) {
        return ParamAccess::get<T>(visitor->m_registry);
      } else if constexpr (is_entity_destroyer<RawT>::value) {
        return ParamAccess::get<T>(visitor->m_registry);
      } else if constexpr (is_component_reader<RawT>::value) {
        return ParamAccess::get<T>(visitor->m_registry);
      } else if constexpr (is_component_writer<RawT>::value) {
        return ParamAccess::get<T>(visitor->m_registry);
      } else if constexpr (is_event_reader<RawT>::value) {
        return ParamAccess::get<T>(visitor->m_event_manager1);
      } else if constexpr (is_event_writer<RawT>::value) {
        return ParamAccess::get<T>(visitor->m_event_manager2);
      }
    }
  }

  template <typename Func, typename... Args>
  void init_impl(Func&& func, std::tuple<Args...>*) {
    m_mutexes = MutexCollector::merge(MutexCollector::get_for_type<Args>()...);
    m_preparers = PreparerCollector::get<Args...>(m_id);

    uint32_t passId = m_id;

    m_binder = [func = std::forward<Func>(func), passId](Visitor<WorldBase>& visitor) mutable -> CallType {
      // Use std::reference_wrapper to ensure correct reference passing
      return [func, params = std::make_tuple(get_param<Args>(visitor, passId)...)]() mutable {
        std::apply(func, params);
      };
    };
  }

  CallType m_execute;
  std::function<CallType(Visitor<WorldBase>&)> m_binder;

  stl::vector<Mutex> m_mutexes;
  stl::vector<std::function<void(Visitor<WorldBase>&)>> m_preparers;

  friend class World;
};

}  // namespace fe::engine
