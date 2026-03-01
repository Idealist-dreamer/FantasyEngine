#pragma once

#include "common.h"

namespace fe::engine::ecs {
struct EntityQuery {
  EntityQuery(Registry& reg) : m_reg(reg) {}

  bool valid(entt::entity e) const { return m_reg.valid(e); }
  auto view() const { return m_reg.view<entt::entity>(); }

 protected:
  Registry& m_reg;
};

struct EntityCreator : public EntityQuery {
  EntityCreator(Registry& reg) : EntityQuery(reg) {}

  entt::entity create() { return m_reg.create(); }
  auto view() { return m_reg.view<entt::entity>(); }
};

struct EntityDestroyer : public EntityCreator {
  EntityDestroyer(Registry& reg) : EntityCreator(reg) {}

  void destroy(entt::entity e) { m_reg.destroy(e); }
};
}  // namespace fe::engine::ecs

namespace fe::engine::ecs {
template <typename T>
struct is_entity_query : std::false_type {};
template <>
struct is_entity_query<EntityQuery> : std::true_type {};

template <typename T>
struct is_entity_creator : std::false_type {};
template <>
struct is_entity_creator<EntityCreator> : std::true_type {};

template <typename T>
struct is_entity_destroyer : std::false_type {};
template <>
struct is_entity_destroyer<EntityDestroyer> : std::true_type {};
}  // namespace fe::engine::ecs