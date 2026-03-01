#pragma once

#include "Engine/ecs/common.h"

namespace fe::engine::ecs {
template <typename... Components>
class ComponentReader {
 public:
  template <typename T>
  static constexpr void check_auth() {
    static_assert((is_compatible<T, Components>::value || ...), "ECS Access Denied: Component or variant not authorized in pack.");
  }

  template <typename... Ts>
  static constexpr void check_auth_all() {
    (check_auth<Ts>(), ...);
  }

  ComponentReader(Registry& reg) : m_reg(reg) {}

  template <typename T>
  bool have(entt::entity e) const {
    check_auth<T>();
    return m_reg.all_of<T>(e);
  }

  template <typename... T>
  bool have_all(entt::entity e) const {
    check_auth_all<T...>();
    return m_reg.all_of<T...>(e);
  }

  template <typename... T>
  bool have_any(entt::entity e) const {
    check_auth_all<T...>();
    return m_reg.any_of<T...>(e);
  }

  template <typename T>
  const T* try_get(entt::entity e) const {
    check_auth<T>();
    return m_reg.try_get<T>(e);
  }

  template <typename T>
  const T& get(entt::entity e) const {
    check_auth<T>();
    return m_reg.get<T>(e);
  }

  auto view() const { return m_reg.view<Components...>(); }

  template <typename... Req, typename = std::enable_if_t<(sizeof...(Req) > 0)>>
  auto view() const {
    (check_auth<Req>(), ...);
    return m_reg.view<Req...>();
  }

 protected:
  Registry& m_reg;
};

template <typename... Components>
class ComponentWriter : public ComponentReader<Components...> {
  using Base = ComponentReader<Components...>;

 public:
  using Base::check_auth;
  using Base::get;
  using Base::have;
  using Base::have_all;
  using Base::have_any;
  using Base::m_reg;
  using Base::try_get;
  using Base::view;

  ComponentWriter(Registry& reg) : Base(reg) {}

  template <typename T>
  T& get(entt::entity e) {
    check_auth<T>();
    return m_reg.get<T>(e);
  }

  auto view() { return m_reg.view<Components...>(); }

  template <typename... Req, typename = std::enable_if_t<(sizeof...(Req) > 0)>>
  auto view() {
    (check_auth<Req>(), ...);
    return m_reg.view<Req...>();
  }

  template <typename T, typename... Args>
  T& add(entt::entity e, Args&&... args) {
    check_auth<T>();
    return m_reg.emplace_or_replace<T>(e, std::forward<Args>(args)...);
  }

  template <typename T>
  bool remove(entt::entity e) {
    check_auth<T>();
    return m_reg.remove<T>(e) > 0;
  }

  template <typename T>
  void clear() {
    check_auth<T>();
    m_reg.clear<T>();
  }

  template <typename T>
  void tag_add(entt::entity e) {
    add<AddComponentTag<T>>(e);
  }
  template <typename T>
  void tag_change(entt::entity e) {
    add<ChangeComponentTag<T>>(e);
  }
  template <typename T>
  void tag_remove(entt::entity e) {
    add<RemoveComponentTag<T>>(e);
  }

  template <typename T, typename... Args>
  void add_delayed(Entity e, Args&&... args) {
    check_auth<T>();
    if (!has<T>(e)) {
      m_reg.remove<ChangeComponentDelayed<T>, RemoveComponentDelayed<T>>(e);
      m_reg.emplace_or_replace<AddComponentDelayed<T>>(e, T{std::forward<Args>(args)...});
    }
  }

  template <typename T, typename... Args>
  void change_delayed(Entity e, Args&&... args) {
    check_auth<T>();
    if (m_reg.try_get<T>(e) && !m_reg.all_of<RemoveComponentDelayed<T>>(e)) {
      m_reg.emplace_or_replace<ChangeComponentDelayed<T>>(e, T{std::forward<Args>(args)...});
    } else if (auto* add_ptr = m_reg.try_get<AddComponentDelayed<T>>(e)) {
      add_ptr->m_data = T(std::forward<Args>(args)...);
    }
  }

  template <typename T>
  void remove_delayed(Entity e) {
    check_auth<T>();
    if (m_reg.try_get<T>(e)) {
      m_reg.remove<AddComponentDelayed<T>, ChangeComponentDelayed<T>>(e);
      m_reg.emplace_or_replace<RemoveComponentDelayed<T>>(e);
    }
  }
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
