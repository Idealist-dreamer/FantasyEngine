#pragma once

#include <entt/entt.hpp>
#include <type_traits>

namespace fe::engine {

// Entt base type
using Entity = entt::entity;
using Registry = entt::registry;

template <typename T>
struct AddComponentTag {};
template <typename T>
struct ChangeComponentTag {};
template <typename T>
struct RemoveEntityTag {};

// Type traits
template <typename T>
struct base_type {
  using type = T;
};
template <typename T>
struct base_type<AddComponentTag<T>> {
  using type = T;
};
template <typename T>
struct base_type<ChangeComponentTag<T>> {
  using type = T;
};
template <typename T>
struct base_type<RemoveEntityTag<T>> {
  using type = T;
};

template <typename T>
using base_type_t = typename base_type<T>::type;

template <typename Req, typename Decl>
struct is_compatible : std::is_same<std::remove_const_t<base_type_t<Req>>,
                                    std::remove_const_t<base_type_t<Decl>>> {};

}  // namespace fe::engine