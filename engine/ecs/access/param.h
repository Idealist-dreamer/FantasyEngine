#pragma once

#include "common.h"
#include "entity.h"
#include "component.h"
#include "resource.h"
#include "event.h"
#include "mutex.h"

#include <type_traits>

namespace fe::engine::ecs::detail {
template <bool IsWrite, typename... Components>
stl::vector<Mutex> collect_comp_mutex_vec(std::tuple<Components...>) {
  if constexpr (IsWrite) {
    return {Mutex::write_component<Components>()...};
  } else {
    return {Mutex::read_component<Components>()...};
  }
}

template <typename T>
stl::vector<Mutex> get_mutexes_for_type() {
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
  } else if constexpr (is_resource_reader<RawT>::value) {
    return {Mutex::read_resource<RawT>()};
  } else if constexpr (is_resource_writer<RawT>::value) {
    return {Mutex::write_resource<RawT>()};
  } else if constexpr (is_event_reader<RawT>::value) {
    return {Mutex::read_event<RawT>()};
  } else if constexpr (is_event_writer<RawT>::value) {
    return {Mutex::write_event<RawT>()};
  }

  return result;
}

template <typename... Vecs>
stl::vector<Mutex> merge_mutex_vectors(Vecs&&... vecs) {
  stl::vector<Mutex> all_mutexes;

  size_t total_size = (vecs.size() + ... + 0);
  all_mutexes.reserve(total_size);

  (all_mutexes.insert(all_mutexes.end(), vecs.begin(), vecs.end()), ...);

  return all_mutexes;
}

template <typename... T>
struct TypeList {};

template <typename... Components>
void prepare_storages(Registry& reg) {
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
void collect_preparers(stl::vector<std::function<void(Registry&)>>& out) {
  using RawT = std::remove_cv_t<std::remove_reference_t<T>>;

  if constexpr (is_component_writer<RawT>::value) {
    out.push_back([](Registry& reg) { Preparer<typename is_component_writer<RawT>::component_types>::run(reg); });
  } else if constexpr (is_component_reader<RawT>::value) {
    out.push_back([](Registry& reg) { Preparer<typename is_component_reader<RawT>::component_types>::run(reg); });
  }
}

template <typename... Args>
auto get_preparers() {
  stl::vector<std::function<void(Registry&)>> preparers;
  (detail::collect_preparers<Args>(preparers), ...);
  return preparers;
}

FE_FINLINE stl::vector<std::function<void(Registry&)>> get_default_preparers() {
  stl::vector<std::function<void(Registry&)>> preparers;

  preparers.push_back([](Registry& reg) {
    reg.storage<DestroyEntityTag>();
    reg.storage<DestroyEntityDelayed>();
  });

  return preparers;
}

}  // namespace fe::engine::ecs::detail