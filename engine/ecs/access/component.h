#pragma once

#include "common.h"

namespace fe::engine::ecs {
template <typename... Components>
class ComponentReader {
 public:
  ComponentReader(Registry& reg) : _reg(reg) {}

  template <typename T>
  bool all_of(entt::entity e) const {
    static_assert(is_in_pack<T, Components...>, "error");
    return _reg.all_of<const T>(e);
  }

  template <typename T>
  const T& get(entt::entity e) const {
    static_assert(is_in_pack<T, Components...>, "error");
    return _reg.get<const T>(e);
  }

  auto view() const { return _reg.view<const Components...>(); }

 protected:
  Registry& _reg;
};

template <typename... Components>
class ComponentWriter {
 public:
  ComponentWriter(entt::registry& reg) : _reg(reg) {}

  template <typename T>
  bool all_of(entt::entity e) const {
    static_assert(is_in_pack<T, Components...>, "error");
    return _reg.all_of<const T>(e);
  }

  template <typename T>
  T& get(entt::entity e) {
    static_assert(is_in_pack<T, Components...>, "error");
    return _reg.get<T>(e);
  }

  template <typename T>
  const T& get(entt::entity e) const {
    static_assert(is_in_pack<T, Components...>, "error");
    return _reg.get<const T>(e);
  }

  auto view() { return _reg.view<const Components...>(); }
  auto view() const { return _reg.view<Components...>(); }

  template <typename T, typename... Args>
  T& add(entt::entity e, Args&&... args) {
    static_assert(is_in_pack<T, Components...>, "error");
    return _reg.emplace_or_replace<T>(e, std::forward<Args>(args)...);
  }

  template <typename T>
  void remove(entt::entity e) {
    static_assert(is_in_pack<T, Components...>, "error");
    _reg.remove<T>(e);
  }

 protected:
  entt::registry& _reg;
};
}  // namespace fe::engine::ecs

namespace fe::engine::ecs {
template <typename T>
struct is_component_reader : std::false_type {};

template <typename... Components>
struct is_component_reader<ComponentReader<Components...>> : std::true_type {
  using component_types = std::tuple<Components...>;
};

template <typename T>
struct is_component_writer : std::false_type {};

template <typename... Components>
struct is_component_writer<ComponentWriter<Components...>> : std::true_type {
  using component_types = std::tuple<Components...>;
};
}  // namespace fe::engine::ecs