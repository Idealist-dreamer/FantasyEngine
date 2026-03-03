#pragma once

#include "engine/base/pch.h"
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

struct EntityCommandBuffer {
  EntityCommandBuffer() {}

  uint32_t create() {
    uint32_t id = m_entity_map.size();
    m_entity_map[id] = entt::null;
    return id;
  }

  bool valid(uint32_t id) const {
    auto it = m_entity_map.find(id);
    return it != m_entity_map.end() && it->second != entt::null;
  }

  entt::entity get(uint32_t id) const { return m_entity_map.at(id); }

  void destroy(entt::entity e) { m_destroyed_entities.push_back(e); }

  void reset() {
    m_entity_map.clear();
    m_destroyed_entities.clear();
  }

 private:
  stl::unordered_map<uint32_t, entt::entity> m_entity_map;
  stl::vector<entt::entity> m_destroyed_entities;

  friend class WorldBase;
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

template <typename T>
struct is_entity_command_buffer : std::false_type {};
template <>
struct is_entity_command_buffer<EntityCommandBuffer> : std::true_type {};
}  // namespace fe::engine::ecs