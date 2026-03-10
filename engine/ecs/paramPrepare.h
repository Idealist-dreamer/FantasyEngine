#pragma once

#include "meta.h"
#include "paramTypes.h"
#include "paramTraits.h"
#include "paramMutex.h"

namespace fe::engine {

// 前向声明
class WorldVisitor;

// ============================================================================
// Mutex collector
// ============================================================================

struct MutexCollector {
  template <bool IsWrite, typename... Components>
  static stl::vector<Mutex> collect_comp_mutex_vec(std::tuple<Components...>) {
    if constexpr (IsWrite) {
      return {Mutex::write_component<Components>()...};
    } else {
      return {Mutex::read_component<Components>()...};
    }
  }

  /// 根据参数类型获取对应的 Mutex 集合
  /// 注意：保留原始类型的 const 属性以正确区分 ContextReader/ContextWriter
  template <typename T>
  static stl::vector<Mutex> get_for_type() {
    using CleanT = meta::clean_t<T>;
    stl::vector<Mutex> result;

    // 优先检查原始类型（保留 const）以区分 Context Reader/Writer
    if constexpr (is_context_reader<T>::value) {
      result.push_back(Mutex::read_context<typename is_context_reader<T>::type>());
    } else if constexpr (is_context_writer<T>::value) {
      result.push_back(Mutex::write_context<typename is_context_writer<T>::type>());
    } else if constexpr (is_entity_query<CleanT>::value) {
      result.push_back(Mutex::query_entity());
    } else if constexpr (is_entity_creator<CleanT>::value) {
      result.push_back(Mutex::create_entity());
    } else if constexpr (is_entity_destroyer<CleanT>::value) {
      result.push_back(Mutex::destroy_entity());
    } else if constexpr (is_component_writer<CleanT>::value) {
      auto m = collect_comp_mutex_vec<true>(typename is_component_writer<CleanT>::component_types{});
      result.insert(result.end(), m.begin(), m.end());
    } else if constexpr (is_component_reader<CleanT>::value) {
      auto m = collect_comp_mutex_vec<false>(typename is_component_reader<CleanT>::component_types{});
      result.insert(result.end(), m.begin(), m.end());
    } else if constexpr (is_event_reader<CleanT>::value) {
      using EvT = typename is_event_reader<CleanT>::type;
      result.push_back(Mutex::read_event<EvT>());
    } else if constexpr (is_event_writer<CleanT>::value) {
      using EvT = typename is_event_writer<CleanT>::type;
      result.push_back(Mutex::write_event<EvT>());
    }

    return result;
  }

  template <typename... Vecs>
  static stl::vector<Mutex> merge(Vecs&&... vecs) {
    stl::vector<Mutex> all;
    size_t total = (vecs.size() + ... + 0);
    all.reserve(total);
    (all.insert(all.end(), vecs.begin(), vecs.end()), ...);
    return all;
  }
};

// ============================================================================
// Preparer collector - 通过 WorldVisitor 接口预先申请资源
// ============================================================================

struct PreparerCollector {
  /// 收集单个参数类型的预处理器
  template <typename T>
  static void collect(stl::vector<std::function<void(WorldVisitor&)>>& out, uint32_t passId) {
    // 优先检查原始类型（保留 const）以区分 Context Reader/Writer
    if constexpr (is_context_reader<T>::value || is_context_writer<T>::value) {
      using U = std::conditional_t<is_context_reader<T>::value, typename is_context_reader<T>::type, typename is_context_writer<T>::type>;
      out.push_back([](WorldVisitor& visitor) { visitor.template prepare_context<U>(); });
    } else {
      using RawT = meta::clean_t<T>;

      if constexpr (is_entity_command_buffer<RawT>::value) {
        out.push_back([passId](WorldVisitor& visitor) { visitor.prepare_entity_command_buffer(passId); });
      } else if constexpr (is_component_writer<RawT>::value) {
        out.push_back([](WorldVisitor& visitor) {
          visitor.template prepare_component_storage_from_tuple<typename is_component_writer<RawT>::component_types>();
        });
      } else if constexpr (is_component_reader<RawT>::value) {
        out.push_back([](WorldVisitor& visitor) {
          visitor.template prepare_component_storage_from_tuple<typename is_component_reader<RawT>::component_types>();
        });
      } else if constexpr (is_event_reader<RawT>::value || is_event_writer<RawT>::value) {
        using EvT = std::conditional_t<is_event_reader<RawT>::value, 
                                        typename is_event_reader<RawT>::type::value_type,
                                        typename is_event_writer<RawT>::type::value_type>;
        out.push_back([](WorldVisitor& visitor) { visitor.template prepare_event<EvT>(); });
      }
    }
  }

  /// 收集所有参数类型的预处理器
  template <typename... Args>
  static stl::vector<std::function<void(WorldVisitor&)>> get(uint32_t passId) {
    stl::vector<std::function<void(WorldVisitor&)>> preparers;
    (collect<Args>(preparers, passId), ...);
    return preparers;
  }
};

}  // namespace fe::engine
