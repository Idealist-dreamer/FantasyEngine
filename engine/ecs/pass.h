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
  /// 参数获取辅助函数：通过 WorldVisitor 接口安全获取参数
  /// 注意：保留原始类型 T 的 const 属性以正确区分 ContextReader/ContextWriter
  template <typename T>
  static auto get_param(WorldVisitor& visitor, uint32_t passId) {
    // 优先检查 context 类型（保留 const）
    if constexpr (is_context_reader<T>::value) {
      using U = typename is_context_reader<T>::type;
      return Context<U>(visitor.template get_context<U>());
    } else if constexpr (is_context_writer<T>::value) {
      using U = typename is_context_writer<T>::type;
      return Context<U>(visitor.template get_context<U>());
    } else {
      using RawT = meta::clean_t<T>;
      if constexpr (is_entity_command_buffer<RawT>::value) {
        // EntityCommandBuffer: 返回引用
        return std::ref(visitor.get_entity_command_buffers()[passId]);
      } else if constexpr (is_entity_query<RawT>::value) {
        return RawT(visitor.get_registry());
      } else if constexpr (is_entity_creator<RawT>::value) {
        return RawT(visitor.get_registry());
      } else if constexpr (is_entity_destroyer<RawT>::value) {
        return RawT(visitor.get_registry());
      } else if constexpr (is_component_reader<RawT>::value) {
        return RawT(visitor.get_registry());
      } else if constexpr (is_component_writer<RawT>::value) {
        return RawT(visitor.get_registry());
      } else if constexpr (is_event_reader<RawT>::value) {
        using EvT = typename is_event_reader<RawT>::type;
        return EventReader<EvT>(*visitor.get_event_manager1()[std::type_index(typeid(EvT))].template get<stl::vector<EvT>>());
      } else if constexpr (is_event_writer<RawT>::value) {
        using EvT = typename is_event_writer<RawT>::type;
        return EventWriter<EvT>(*visitor.get_event_manager2()[std::type_index(typeid(EvT))].template get<stl::vector<EvT>>());
      }
    }
  }

  template <typename Func, typename... Args>
  void init_impl(Func&& func, std::tuple<Args...>*) {
    m_mutexes = MutexCollector::merge(MutexCollector::get_for_type<Args>()...);
    m_preparers = PreparerCollector::get<Args...>(m_id);

    uint32_t passId = m_id;

    // 使用 WorldVisitor 接口进行参数绑定
    m_binder = [func = std::forward<Func>(func), passId](WorldVisitor& visitor) mutable -> CallType {
      // 使用 std::reference_wrapper 确保正确传递引用
      return [func, params = std::make_tuple(get_param<Args>(visitor, passId)...)]() mutable {
        std::apply(func, params);
      };
    };
  }

  CallType m_execute;
  std::function<CallType(WorldVisitor&)> m_binder;

  stl::vector<Mutex> m_mutexes;
  stl::vector<std::function<void(WorldVisitor&)>> m_preparers;

  friend class World;
};

}  // namespace fe::engine
