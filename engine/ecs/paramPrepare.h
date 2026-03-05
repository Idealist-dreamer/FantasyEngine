#pragma once

#include "paramTypes.h"
#include "paramTraits.h"
#include "paramMutex.h"
#include "meta.h"

namespace fe::engine::ecs {

class WorldBase;

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

  template <typename T>
  static stl::vector<Mutex> get_for_type() {
    using RawT = meta::clean_t<T>;
    stl::vector<Mutex> result;

    if constexpr (is_entity_query<RawT>::value) {
      result.push_back(Mutex::query_entity());
    } else if constexpr (is_entity_creator<RawT>::value) {
      result.push_back(Mutex::create_entity());
    } else if constexpr (is_entity_destroyer<RawT>::value) {
      result.push_back(Mutex::destroy_entity());
    } else if constexpr (is_component_writer<RawT>::value) {
      auto m = collect_comp_mutex_vec<true>(typename is_component_writer<RawT>::component_types{});
      result.insert(result.end(), m.begin(), m.end());
    } else if constexpr (is_component_reader<RawT>::value) {
      auto m = collect_comp_mutex_vec<false>(typename is_component_reader<RawT>::component_types{});
      result.insert(result.end(), m.begin(), m.end());
    } else if constexpr (is_event_reader<RawT>::value) {
      using EvT = typename is_event_reader<RawT>::type;
      result.push_back(Mutex::read_event<EvT>());
    } else if constexpr (is_event_writer<RawT>::value) {
      using EvT = typename is_event_writer<RawT>::type;
      result.push_back(Mutex::write_event<EvT>());
    } else if constexpr (is_resource_reader<RawT>::value) {
      result.push_back(Mutex::read_resource<typename RawT::type>());
    } else if constexpr (is_resource_writer<RawT>::value) {
      result.push_back(Mutex::write_resource<typename RawT::type>());
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
// Storage preparer
// ============================================================================

struct StoragePreparer {
  template <typename... Components>
  static void prepare(Registry& reg) {
    (reg.template storage<std::remove_const_t<Components>>(), ...);
    (reg.template storage<AddComponentTag<Components>>(), ...);
    (reg.template storage<ChangeComponentTag<Components>>(), ...);
    (reg.template storage<RemoveComponentTag<Components>>(), ...);
    (reg.template storage<AddComponentDelayed<Components>>(), ...);
    (reg.template storage<ChangeComponentDelayed<Components>>(), ...);
    (reg.template storage<RemoveComponentDelayed<Components>>(), ...);
  }

  template <typename Tuple>
  struct ForTuple;

  template <typename... Cs>
  struct ForTuple<std::tuple<Cs...>> {
    static void run(Registry& reg) { prepare<Cs...>(reg); }
  };
};

// ============================================================================
// Preparer collector
// ============================================================================

struct PreparerCollector {
  template <typename T>
  static void collect(stl::vector<std::function<void(WorldBase&)>>& out) {
    using RawT = meta::clean_t<T>;

    if constexpr (is_component_writer<RawT>::value) {
      out.push_back([](WorldBase& w) {
        StoragePreparer::ForTuple<typename is_component_writer<RawT>::component_types>::run(w.m_registry);
      });
    } else if constexpr (is_component_reader<RawT>::value) {
      out.push_back([](WorldBase& w) {
        StoragePreparer::ForTuple<typename is_component_reader<RawT>::component_types>::run(w.m_registry);
      });
    } else if constexpr (is_event_reader<RawT>::value || is_event_writer<RawT>::value) {
      using EvT = std::conditional_t<is_event_reader<RawT>::value, typename is_event_reader<RawT>::type,
                                     typename is_event_writer<RawT>::type>;
      out.push_back([](WorldBase& w) {
        auto tid = std::type_index(typeid(EvT));
        if (w.m_event_manager1.find(tid) == w.m_event_manager1.end()) {
          w.m_event_manager1.insert({tid, ContextStorage::create<EvT>()});
          w.m_event_manager2.insert({tid, ContextStorage::create<EvT>()});
          w.m_event_swap[tid] = [](WorldBase& wb) {
            auto inner_tid = std::type_index(typeid(EvT));
            auto& data1 = *(wb.m_event_manager1[inner_tid].template get<EvT>());
            auto& data2 = *(wb.m_event_manager2[inner_tid].template get<EvT>());
            data1.swap(data2);
            data2.clear();
          };
        }
      });
    } else if constexpr (is_resource_reader<RawT>::value || is_resource_writer<RawT>::value) {
      using U = typename RawT::type;
      out.push_back([](WorldBase& w) {
        w.m_resource_manager.insert({std::type_index(typeid(U)), ContextStorage()});
      });
    }
  }

  template <typename... Args>
  static stl::vector<std::function<void(WorldBase&)>> get() {
    stl::vector<std::function<void(WorldBase&)>> preparers;
    (collect<Args>(preparers), ...);
    return preparers;
  }
};

}  // namespace fe::engine::ecs
