#pragma once

#include "common.h"

namespace fe::engine::ecs {
class ResourceManager;

template <typename T>
struct is_resource_manager : std::false_type {};

template <>
struct is_resource_manager<ResourceManager&> : std::true_type {
  static constexpr bool is_write = true;
};

template <>
struct is_resource_manager<const ResourceManager&> : std::true_type {
  static constexpr bool is_write = false;
};
}  // namespace fe::engine::ecs