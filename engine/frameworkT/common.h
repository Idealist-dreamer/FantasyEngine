#pragma once

#include <entt/entt.hpp>

#include "foundation/container/stl.h"
#include "foundation/utility/any.h"

namespace fe::engine {

// Entity types
using Entity = entt::entity;
using Registry = entt::registry;

// Type erase helper
template <typename T>
using clean_t = std::remove_cv_t<std::remove_reference_t<T>>;

// Priority levels
enum Priority : uint32_t { First = 0, High = 1000, Mid = 2000, Low = 3000 };

// Component tags for delayed operations
template <typename T>
struct AddTag {};
template <typename T>
struct ChangeTag {};
template <typename T>
struct RemoveTag {};

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

// Base type extraction
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

// Component compatibility check
template <typename Req, typename Decl>
struct is_compatible : std::is_same<std::remove_const_t<base_type_t<Req>>,
                                    std::remove_const_t<base_type_t<Decl>>> {};

}  // namespace fe::engine
