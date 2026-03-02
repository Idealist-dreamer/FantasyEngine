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
      return collect_comp_mutex_vec<true>(typename is_component_writer<RawT>::component_types{});
    } else if constexpr (is_component_reader<RawT>::value) {
      return collect_comp_mutex_vec<false>(typename is_component_reader<RawT>::component_types{});
    } else if constexpr (is_event_reader<RawT>::value) {
      return {Mutex::read_event<RawT>()};
    } else if constexpr (is_event_writer<RawT>::value) {
      return {Mutex::write_event<RawT>()};
    } else {
      if constexpr (std::is_const_v<NoRefT>) {
        return {Mutex::read_resource<RawT>()};
      } else {
        return {Mutex::write_resource<RawT>()};
      }
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
      using EvT = typename is_event_reader<RawT>::type;
      out.push_back([](WorldBase& w) { w.m_event_manager1.insert({std::type_index(typeid(RawT)), ResourceStorage()}); });
      out.push_back([](WorldBase& w) { w.m_event_manager2.insert({std::type_index(typeid(RawT)), ResourceStorage()}); });
    } else {
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

    preparers.push_back([](WorldBase& world) {
      world.m_registry.storage<DestroyEntityTag>();
      world.m_registry.storage<DestroyEntityDelayed>();
    });

    return preparers;
  }
};

}  // namespace fe::engine::ecs