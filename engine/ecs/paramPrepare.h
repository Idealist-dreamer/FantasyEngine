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

  // Fix: Check original type T (preserving const) for Context Reader/Writer distinction
  // Reason: ContextReader = const Context<T>, clean_t removes const
  // causing is_resource_reader to fail, misidentifying as writer
  template <typename T>
  static stl::vector<Mutex> get_for_type() {
    using CleanT = meta::clean_t<T>;
    stl::vector<Mutex> result;

    // Check original type first (preserving const) for Context Reader/Writer
    if constexpr (is_resource_reader<T>::value) {
      result.push_back(Mutex::read_resource<typename is_resource_reader<T>::type>());
    } else if constexpr (is_resource_writer<T>::value) {
      result.push_back(Mutex::write_resource<typename is_resource_writer<T>::type>());
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
    // Check original type first (preserving const) for Context Reader/Writer
    if constexpr (is_resource_reader<T>::value || is_resource_writer<T>::value) {
      using U = std::conditional_t<is_resource_reader<T>::value,
                                   typename is_resource_reader<T>::type,
                                   typename is_resource_writer<T>::type>;
      out.push_back([](WorldBase& w) {
        w.m_resource_manager.insert({std::type_index(typeid(U)), ContextStorage()});
      });
    } else {
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
      }
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
