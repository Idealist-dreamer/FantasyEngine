#pragma once

#include "common.h"
#include "context.h"

namespace fe::engine::ecs {
template <typename T>
struct Context {
  using type = T;

  Context(ContextStorage& res) : m_resource(res) {}

  bool valid() const { return m_resource.valid(); }

  T& get() { return *m_resource.get<T>(); }
  const T& get() const { return *m_resource.get<T>(); }

  void destroy() { m_resource.destroy(); }

  template <typename... Args>
  void create(Args&&... args) {
    m_resource = ContextStorage::create<T>(std::forward<Args>(args)...);
  }

  void create(T* ptr, bool need_free = true) { m_resource = ContextStorage::create<T>(ptr, need_free); }

 private:
  ContextStorage& m_resource;
};

template <typename T>
using ContextReader = const Context<T>;

template <typename T>
using ContextWriter = Context<T>;
}  // namespace fe::engine::ecs

namespace fe::engine::ecs {
template <typename T>
struct is_resource_reader : std::false_type {};

template <typename T>
struct is_resource_reader<ContextReader<T>> : std::true_type {
  using type = T;
};

template <typename T>
struct is_resource_writer : std::false_type {};

template <typename T>
struct is_resource_writer<ContextWriter<T>> : std::true_type {
  using type = T;
};
}  // namespace fe::engine::ecs