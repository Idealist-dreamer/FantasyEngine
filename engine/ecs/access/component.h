#pragma once

#include "Engine/ecs/common.h"
#include "Engine/ecs/access/comTag.h"

namespace fe::engine::ecs {
template <typename... Components>
class ComponentReader {
 public:
  template <typename T>
  static constexpr bool is_auth_v = (is_compatible<T, Components>::value || ...);

  template <typename T>
  static constexpr void check_auth() {
    static_assert(is_auth_v<T>, "ECS Access Denied: Component or variant not authorized in pack.");
  }

  ComponentReader(Registry& reg) : _reg(reg) {}

  template <typename T>
  bool has(entt::entity e) const {
    check_auth<T>();
    return _reg.all_of<const T>(e);
  }

  template <typename T>
  const T& get(entt::entity e) const {
    check_auth<T>();
    return _reg.get<const T>(e);
  }

  auto view() const { return _reg.view<const Components...>(); }

  template <typename... Req, typename = std::enable_if_t<(sizeof...(Req) > 0)>>
  auto view() const {
    (check_auth<Req>(), ...);
    return _reg.view<const Req...>();
  }

 protected:
  Registry& _reg;
};

template <typename... Components>
class ComponentWriter : public ComponentReader<Components...> {
  using Base = ComponentReader<Components...>;

 public:
  using Base::_reg;
  using Base::check_auth;
  using Base::get;
  using Base::has;
  using Base::view;

  ComponentWriter(entt::registry& reg) : Base(reg) {}

  template <typename T>
  T& get(entt::entity e) {
    check_auth<T>();
    return _reg.get<T>(e);
  }

  auto view() { return _reg.view<Components...>(); }

  template <typename... Req, typename = std::enable_if_t<(sizeof...(Req) > 0)>>
  auto view() {
    (check_auth<Req>(), ...);
    return _reg.view<Req...>();
  }

  template <typename T, typename... Args>
  T& add(entt::entity e, Args&&... args) {
    check_auth<T>();
    return _reg.emplace_or_replace<T>(e, std::forward<Args>(args)...);
  }

  template <typename T>
  bool remove(entt::entity e) {
    check_auth<T>();
    return _reg.remove<T>(e) > 0;
  }

  template <typename T>
  void clear() {
    check_auth<T>();
    _reg.clear<T>();
  }

  template <typename T>
  void tagAdd(entt::entity e) {
    add<AddComponentTag<T>>(e);
  }
  template <typename T>
  void tagChange(entt::entity e) {
    add<ChangeComponentTag<T>>(e);
  }
  template <typename T>
  void tagRemove(entt::entity e) {
    add<RemoveComponentTag<T>>(e);
  }

  template <typename T, typename... Args>
  void addDelayed(Entity e, Args&&... args) {
    check_auth<T>();
    if (!has<T>(e)) {
      _reg.remove<ChangeComponentDelayed<T>, RemoveComponentDelayed<T>>(e);
      _reg.emplace_or_replace<AddComponentDelayed<T>>(e, T{std::forward<Args>(args)...});
    }
  }

  template <typename T, typename... Args>
  void changeDelayed(Entity e, Args&&... args) {
    check_auth<T>();
    if (_reg.try_get<T>(e) && !_reg.all_of<RemoveComponentDelayed<T>>(e)) {
      _reg.emplace_or_replace<ChangeComponentDelayed<T>>(e, T{std::forward<Args>(args)...});
    } else if (auto* add_ptr = _reg.try_get<AddComponentDelayed<T>>(e)) {
      add_ptr->data = T(std::forward<Args>(args)...);
    }
  }

  template <typename T>
  void removeDelayed(Entity e) {
    check_auth<T>();
    if (_reg.try_get<T>(e)) {
      _reg.remove<AddComponentDelayed<T>, ChangeComponentDelayed<T>>(e);
      _reg.emplace_or_replace<RemoveComponentDelayed<T>>(e);
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