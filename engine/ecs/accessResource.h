#pragma once

#include "common.h"
#include "resource.h"

namespace fe::engine::ecs {
template <typename T>
struct Resource {
  Resource(ResourceStorage& res) : m_resource(res) {}

  bool valid() const { return m_resource.valid(); }

  T& get() { return *m_resource.get<T>(); }
  const T& get() const { return *m_resource.get<T>(); }

  void destroy() { m_resource.destroy(); }

  template <typename... Args>
  void create(Args&&... args) {
    m_resource = ResourceStorage::create<T>(std::forward<Args>(args)...);
  }

  void create(T* ptr, bool need_free = true) { m_resource = ResourceStorage::create<T>(ptr, need_free); }

 private:
  ResourceStorage& m_resource;
};

template <typename T>
using ResourceReader = const Resource<T>;

template <typename T>
using ResourceWriter = Resource<T>;
}  // namespace fe::engine::ecs

namespace fe::engine::ecs {
template <typename T>
struct is_resource_reader : std::false_type {};

template <typename T>
struct is_resource_reader<ResourceReader<T>> : std::true_type {
  using type = T;
};

template <typename T>
struct is_resource_writer : std::false_type {};

template <typename T>
struct is_resource_writer<ResourceWriter<T>> : std::true_type {
  using type = T;
};
}  // namespace fe::engine::ecs