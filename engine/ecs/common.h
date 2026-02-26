#pragma once

#include <entt/entt.hpp>

namespace fe::engine::ecs {
using Entity = entt::entity;
using Registry = entt::registry;

template <typename T>
struct AddComponentTag {
  T data;
};

template <typename T>
struct ChangeComponentTag {
  T data;
};

template <typename T>
struct RemoveComponentTag {};

struct DestroyEntityTag {};
}  // namespace fe::engine::ecs