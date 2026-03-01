#pragma once

#include "common.h"

namespace fe::engine::ecs {
template <typename T>
class ResourceReader {
 public:
  ResourceReader(const T& _value) : value(_value) {}

  const T& get() const { return value; }

 private:
  const T& value;
};

template <typename T>
class ResourceWriter {
 public:
  ResourceWriter(T& _value) : value(_value) {}

  const T& get() const { return value; }
  T& get() const { return value; }

 private:
  T& value;
};

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