#pragma once

#include <entt/entt.hpp>

#include <functional>

#include "foundation/utility/any.h"
#include "foundation/container/stl.h"

namespace fe::engine {

using Entity = entt::entity;
using Registry = entt::registry;

struct EntityQuery {
  EntityQuery(Registry& reg) : m_reg(reg) {}

  bool valid(Entity e) const { return m_reg.valid(e); }
  auto view() const { return m_reg.view<Entity>(); }

 protected:
  Registry& m_reg;
};

struct EntityCreator : EntityQuery {
  EntityCreator(Registry& reg) : EntityQuery(reg) {}

  Entity create() { return m_reg.create(); }
  auto view() { return m_reg.view<Entity>(); }
};

struct EntityDestroyer : EntityCreator {
  EntityDestroyer(Registry& reg) : EntityCreator(reg) {}

  void destroy(Entity e) { m_reg.destroy(e); }
};

struct ECCommandBuffer {
  slt::vector<std::function<void(Registry&)>> m_commands;
};

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