#pragma once

#include "worldBase.h"

#include <type_traits>

namespace fe::engine::ecs {
class WorldBase;

class Detail {
 public:
  template <bool IsWrite, typename... Components>
  static stl::vector<Mutex> collect_comp_mutex_vec(std::tuple<Components...>) {
    if constexpr (IsWrite) {
      return {Mutex::write_component<Components>()...};
    } else {
      return {Mutex::read_component<Components>()...};
    }
  }

  template <typename T>
  static stl::vector<Mutex> get_mutexes_for_type() {
    using NoRefT = std::remove_reference_t<T>;
    using RawT = std::remove_cv_t<std::remove_reference_t<T>>;

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
      result.push_back(Mutex::read_event<RawT>());
    } else if constexpr (is_event_writer<RawT>::value) {
      result.push_back(Mutex::write_event<RawT>());
    } else if constexpr (is_resource_reader<T>::value) {
      result.push_back(Mutex::read_resource<RawT>());
    } else if constexpr (is_resource_writer<T>::value) {
      result.push_back(Mutex::write_resource<RawT>());
    }

    return result;
  }

  template <typename... Vecs>
  static stl::vector<Mutex> merge_mutex_vectors(Vecs&&... vecs) {
    stl::vector<Mutex> all_mutexes;

    size_t total_size = (vecs.size() + ... + 0);
    all_mutexes.reserve(total_size);

    (all_mutexes.insert(all_mutexes.end(), vecs.begin(), vecs.end()), ...);

    return all_mutexes;
  }

  template <typename... T>
  struct TypeList {};

  template <typename... Components>
  static void prepare_storages(Registry& reg) {
    (reg.storage<std::remove_const_t<Components>>(), ...);
    (reg.storage<AddComponentTag<Components>>(), ...);
    (reg.storage<ChangeComponentTag<Components>>(), ...);
    (reg.storage<RemoveComponentTag<Components>>(), ...);
    (reg.storage<AddComponentDelayed<Components>>(), ...);
    (reg.storage<ChangeComponentDelayed<Components>>(), ...);
    (reg.storage<RemoveComponentDelayed<Components>>(), ...);
  }

  template <typename Tuple>
  struct Preparer;

  template <typename... Cs>
  struct Preparer<std::tuple<Cs...>> {
    static void run(Registry& reg) { prepare_storages<Cs...>(reg); }
  };

  template <typename T>
  static void collect_preparers(stl::vector<std::function<void(WorldBase&)>>& out) {
    using RawT = std::remove_cv_t<std::remove_reference_t<T>>;

    if constexpr (is_component_writer<RawT>::value) {
      out.push_back([](WorldBase& w) { Preparer<typename is_component_writer<RawT>::component_types>::run(w.m_registry); });
    } else if constexpr (is_component_reader<RawT>::value) {
      out.push_back([](WorldBase& w) { Preparer<typename is_component_reader<RawT>::component_types>::run(w.m_registry); });
    } else if constexpr (is_event_reader<RawT>::value || is_event_writer<RawT>::value) {
      using EvT = std::conditional_t<is_event_reader<RawT>::value, typename is_event_reader<RawT>::type, typename is_event_writer<RawT>::type>;

      out.push_back([](WorldBase& w) {
        auto tid = std::type_index(typeid(EvT));

        if (w.m_event_manager1.find(tid) == w.m_event_manager1.end()) {
          w.m_event_manager1.insert({tid, ResourceStorage::create<EvT>()});
          w.m_event_manager2.insert({tid, ResourceStorage::create<EvT>()});

          w.m_event_swap[tid] = [](WorldBase& wb) {
            auto inner_tid = std::type_index(typeid(EvT));
            auto& data1 = *(wb.m_event_manager1[inner_tid].get<EvT>());
            auto& data2 = *(wb.m_event_manager2[inner_tid].get<EvT>());

            data1.swap(data2);
            data2.clear();
          };
        }
      });
    } else if constexpr (is_resource_reader<T>::value || is_resource_writer<T>::value) {
      out.push_back([](WorldBase& w) { w.m_resource_manager.insert({std::type_index(typeid(RawT)), ResourceStorage()}); });
    }
  }

  template <typename... Args>
  static auto get_preparers() {
    stl::vector<std::function<void(WorldBase&)>> preparers;
    (collect_preparers<Args>(preparers), ...);
    return preparers;
  }

  FE_FINLINE static stl::vector<std::function<void(WorldBase&)>> get_default_preparers() {
    stl::vector<std::function<void(WorldBase&)>> preparers;

    return preparers;
  }
};

template <typename T>
struct function_traits : function_traits<decltype(&T::operator())> {};

template <typename R, typename... Args>
struct function_traits<R (*)(Args...)> {
  using args_tuple = std::tuple<Args...>;
};

template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const> {
  using args_tuple = std::tuple<Args...>;
};

template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...)> {
  using args_tuple = std::tuple<Args...>;
};
}  // namespace fe::engine::ecs