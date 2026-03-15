#pragma once

#include <entt/entt.hpp>
#include <type_traits>

namespace fe::engine {

// Entt base type
using Entity = entt::entity;
using Registry = entt::registry;

template <typename T>
struct AddTag {};
template <typename T>
struct ChangeTag {};
template <typename T>
struct RemoveTag {};

// Delayed tag component types
template <typename T>
struct AddDelayed {
  T data;
};
template <typename T>
struct ChangeDelayed {
  T data;
};
template <typename T>
struct RemoveDelayed {};

// Type traits
template <typename T>
struct base_type {
  using type = T;
};
template <typename T>
struct base_type<AddTag<T>> {
  using type = T;
};
template <typename T>
struct base_type<ChangeTag<T>> {
  using type = T;
};
template <typename T>
struct base_type<RemoveTag<T>> {
  using type = T;
};
template <typename T>
struct base_type<AddDelayed<T>> {
  using type = T;
};
template <typename T>
struct base_type<ChangeDelayed<T>> {
  using type = T;
};
template <typename T>
struct base_type<RemoveDelayed<T>> {
  using type = T;
};

template <typename T>
using base_type_t = typename base_type<T>::type;

template <typename Req, typename Decl>
struct is_compatible : std::is_same<std::remove_const_t<base_type_t<Req>>,
                                    std::remove_const_t<base_type_t<Decl>>> {};

}  // namespace fe::engine