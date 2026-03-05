#pragma once

#include <entt/entt.hpp>

namespace fe::engine::ecs {
using Entity = entt::entity;
using Registry = entt::registry;

// Special component type
template <typename T>
struct AddComponentTag {};

template <typename T>
struct ChangeComponentTag {};

template <typename T>
struct RemoveComponentTag {};

template <typename T>
struct AddComponentDelayed {
  T m_data;
};

template <typename T>
struct ChangeComponentDelayed {
  T m_data;
};

template <typename T>
struct RemoveComponentDelayed {};

// Special component decide
template <typename T>
struct get_base_type {
  using type = T;
};

template <typename T>
struct get_base_type<AddComponentTag<T>> {
  using type = T;
};
template <typename T>
struct get_base_type<ChangeComponentTag<T>> {
  using type = T;
};
template <typename T>
struct get_base_type<RemoveComponentTag<T>> {
  using type = T;
};

template <typename T>
struct get_base_type<AddComponentDelayed<T>> {
  using type = T;
};
template <typename T>
struct get_base_type<ChangeComponentDelayed<T>> {
  using type = T;
};
template <typename T>
struct get_base_type<RemoveComponentDelayed<T>> {
  using type = T;
};

template <typename T>
using base_type_t = typename get_base_type<T>::type;

template <typename Requested, typename Declared>
struct is_compatible : std::is_same<std::remove_const_t<base_type_t<Requested>>, std::remove_const_t<base_type_t<Declared>>> {};
}  // namespace fe::engine::ecs