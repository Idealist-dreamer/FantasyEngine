#pragma once

#include "common.h"
#include "transform.h"

#include "resource/resourceManager.h"

namespace fe::engine::ecs {
class WorldBase {
 public:
  WorldBase();
  virtual ~WorldBase();

  FE_FINLINE bool hasEntity(Entity e) const { return m_reg.valid(e); }
  FE_FINLINE Entity createEntity() {
    Entity e = m_reg.create();
    m_reg.emplace<Transform>(e);
    m_reg.emplace<ModelMatrix>(e);
    m_reg.emplace<Hierarchy>(e);
    return e;
  }

  FE_FINLINE void destroyEntity(Entity e) { m_reg.destroy(e); }
  FE_FINLINE void destroyEntityDelayed(Entity e) { m_reg.emplace_or_replace<DestroyEntityTag>(e); }

  template <typename T>
  bool hasComponents(Entity e) const {
    return m_reg.all_of<T>(e);
  }

  template <typename T>
  T& getComponent(Entity e) {
    return m_reg.get<T>(e);
  }

  template <typename T>
  const T& getComponent(Entity e) const {
    return m_reg.get<T>(e);
  }

  template <typename T, typename... Args>
  void addComponent(Entity e, Args&&... args) {
    m_reg.emplace_or_replace<T>(e, std::forward<Args>(args)...);
  }

  template <typename T, typename... Args>
  void addComponentDelayed(Entity e, Args&&... args) {
    if (!hasComponents<T>(e)) {
      m_reg.remove<ChangeComponentTag<T>>(e);
      m_reg.remove<RemoveComponentTag<T>>(e);

      AddComponentTag<T> comTag = {std::forward<Args>(args)...};
      m_reg.emplace_or_replace<AddComponentTag<T>>(e, comTag);
    }
  }

  template <typename T, typename... Args>
  void changeComponent(Entity e, Args&&... args) {
    m_reg.get<T>(e) = T(std::forward<Args>(args)...);
  }

  template <typename T, typename... Args>
  void changeComponentDelayed(Entity e, Args&&... args) {
    if (m_reg.try_get<T>(e) && !m_reg.all_of<RemoveComponentTag<T>>(e)) {
      ChangeComponentTag<T> comTag = {std::forward<Args>(args)...};
      m_reg.emplace_or_replace<ChangeComponentTag<T>>(e, comTag);
    } else if (m_reg.try_get<AddComponentTag<T>>(e)) {
      m_reg.get<AddComponentTag<T>>(e).data = T(std::forward<Args>(args)...);
    }
  }

  template <typename T>
  void removeComponent(Entity e) {
    m_reg.remove<T>(e);
  }

  template <typename T>
  void removeComponentDelayed(Entity e) {
    if (m_reg.try_get<T>(e)) {
      m_reg.remove<AddComponentTag<T>>(e);
      m_reg.remove<ChangeComponentTag<T>>(e);

      m_reg.emplace_or_replace<RemoveComponentTag<T>>(e);
    }
  }

  template <typename... Component>
  auto view() {
    return m_reg.view<Component...>();
  }

  template <typename... Component>
  auto view() const {
    return m_reg.view<const Component...>();
  }

  FE_FINLINE Registry* registry() { return &m_reg; }
  FE_FINLINE const Registry* registry() const { return &m_reg; }

  FE_FINLINE ResourceManager* resourceManager() { return m_resourceManager.get(); }
  FE_FINLINE const ResourceManager* resourceManager() const { return m_resourceManager.get(); }

 protected:
  Registry m_reg;
  stl::unique_ptr<ResourceManager> m_resourceManager;
};
}  // namespace fe::engine::ecs