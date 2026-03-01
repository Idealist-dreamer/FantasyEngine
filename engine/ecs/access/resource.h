#pragma once

#include "common.h"

namespace fe::engine::ecs {
class Resource;

template <typename T>
class ResourceReader {
 public:
  ResourceReader(Resource& resource) : m_resource(resource) {}

  const Resource& get() const { return m_resource; }

 protected:
  Resource m_resource;
};

template <typename T>
class ResourceWriter : public ResourceReader<T> {
 public:
  using ResourceReader<T>::get;
  using ResourceReader<T>::m_resource;

  ResourceWriter(Resource& resource) : ResourceReader<T>(resource) {}

  Resource& get() { return m_resource; }
};
}  // namespace fe::engine::ecs

namespace fe::engine::ecs {
template <typename T>
struct is_resource_reader : std::false_type {};
template <typename T>
struct is_resource_reader<ResourceReader<T>> : std::true_type {
  using Type = T;
};

template <typename T>
struct is_resource_writer : std::false_type {};
template <typename T>
struct is_resource_writer<ResourceWriter<T>> : std::true_type {
  using Type = T;
};
}  // namespace fe::engine::ecs