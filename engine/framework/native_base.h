#pragma once

#include <entt/entt.hpp>
#include <type_traits>

namespace fe::engine {

// Entt base type
using Entity = entt::entity;
using Registry = entt::registry;

// ============================================================================
// Tag types for immediate component operations
// ============================================================================
template <typename T>
struct AddComponentTag {};
template <typename T>
struct ChangeComponentTag {};
template <typename T>
struct RemoveComponentTag {};

// Aliases for shorter usage
template <typename T>
using AddTag = AddComponentTag<T>;
template <typename T>
using ChangeTag = ChangeComponentTag<T>;
template <typename T>
using RemoveTag = RemoveComponentTag<T>;

// ============================================================================
// Delayed types for deferred component operations
// ============================================================================
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

// Aliases for shorter usage
template <typename T>
using AddDelayed = AddComponentDelayed<T>;
template <typename T>
using ChangeDelayed = ChangeComponentDelayed<T>;
template <typename T>
using RemoveDelayed = RemoveComponentDelayed<T>;

// ============================================================================
// Type traits
// ============================================================================
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
struct base_type<RemoveComponentTag<T>> {
  using type = T;
};
template <typename T>
struct base_type<AddComponentDelayed<T>> {
  using type = T;
};
template <typename T>
struct base_type<ChangeComponentDelayed<T>> {
  using type = T;
};
template <typename T>
struct base_type<RemoveComponentDelayed<T>> {
  using type = T;
};

template <typename T>
using base_type_t = typename base_type<T>::type;

template <typename Req, typename Decl>
struct is_compatible : std::is_same<std::remove_const_t<base_type_t<Req>>,
                                    std::remove_const_t<base_type_t<Decl>>> {};

}  // namespace fe::engine
