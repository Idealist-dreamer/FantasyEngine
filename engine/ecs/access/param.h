#pragma once

#include "common.h"

#include "entity.h"
#include "component.h"
#include "resource.h"

#include "mutex.h"

#include <type_traits>

namespace fe::engine::ecs::detail {
template <bool IsWrite, typename... Components>
stl::vector<Mutex> collect_comp_mutex_vec(std::tuple<Components...>) {
  if constexpr (IsWrite) {
    return {Mutex::WriteComponent<Components>()...};
  } else {
    return {Mutex::ReadComponent<Components>()...};
  }
}

template <typename T>
stl::vector<Mutex> get_mutexes_for_type() {
  using RawT = std::remove_cv_t<std::remove_reference_t<T>>;
  stl::vector<Mutex> result;

  if constexpr (is_entity_query<RawT>::value) {
    result.push_back(Mutex::QueryEntity());
  } else if constexpr (is_entity_creator<RawT>::value) {
    result.push_back(Mutex::CreateEntity());
  } else if constexpr (is_entity_destroyer<RawT>::value) {
    result.push_back(Mutex::DestroyEntity());
  } else if constexpr (is_component_writer<RawT>::value) {
    return collect_comp_mutex_vec<true>(typename is_component_writer<RawT>::component_types{});
  } else if constexpr (is_component_reader<RawT>::value) {
    return collect_comp_mutex_vec<false>(typename is_component_reader<RawT>::component_types{});
  } else if constexpr (is_resource_manager<T>::value) {
    if constexpr (is_resource_manager<T>::is_write)
      result.push_back(Mutex::UseClassNoConst<ResourceManager>());
    else
      result.push_back(Mutex::UseClassConst<ResourceManager>());
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

}  // namespace fe::engine::ecs::detail