#pragma once

#include "common.h"

namespace fe::engine::ecs {
struct EntityQuery {
  EntityQuery(Registry& reg) : _reg(reg) {}

  bool valid(entt::entity e) const { return _reg.valid(e); }
  auto view() const { return _reg.view<entt::entity>(); }

 protected:
  Registry& _reg;
};

struct EntityCreator {
  EntityCreator(Registry& reg) : _reg(reg) {}

  bool valid(entt::entity e) const { return _reg.valid(e); }
  auto view() const { return _reg.view<entt::entity>(); }

  entt::entity create() { return _reg.create(); }
  auto view() { return _reg.view<entt::entity>(); }

 protected:
  Registry& _reg;
};

struct EntityDestroyer {
  EntityDestroyer(Registry& reg) : _reg(reg) {}

  bool valid(entt::entity e) const { return _reg.valid(e); }
  auto view() const { return _reg.view<entt::entity>(); }

  entt::entity create() { return _reg.create(); }
  auto view() { return _reg.view<entt::entity>(); }

  void destroy(entt::entity e) { _reg.destroy(e); }

 protected:
  Registry& _reg;
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