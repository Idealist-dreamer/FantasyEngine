#pragma once

#include "worldBase.h"

namespace fe::engine::ecs {
struct PM_Exclusive {
  static constexpr PassMutex to_mutex() { return PassMutex(MutexType::Exclusive); }
};
using OnlyExe = PM_Exclusive;

template <AccessType AT>
struct PM_Entity {
  static constexpr PassMutex to_mutex() { return PassMutex(MutexType::EntityAccess, AT); }
};
using ReadEntity = PM_Entity<AccessType::Read>;
using WriteEntity = PM_Entity<AccessType::ReadWrite>;

template <typename T, AccessType AT>
struct PM_Component {
  static constexpr PassMutex to_mutex() { return PassMutex(MutexType::EntityAccess, AT, typeid(T).hash_code()); }
};
template <typename T>
using ReadComponent = PM_Component<T, AccessType::Read>;
template <typename T>
using WriteComponent = PM_Component<T, AccessType::ReadWrite>;

template <typename T, AccessType AT>
struct PM_Class {
  static constexpr PassMutex to_mutex() { return PassMutex(MutexType::UseClass, AT, typeid(T).hash_code()); }
};
template <typename T>
using ReadClass = PM_Class<T, AccessType::Read>;
template <typename T>
using WriteClass = PM_Class<T, AccessType::ReadWrite>;

#define FE_USE_RM_CONST                                                                                                                \
  static_assert((std::is_same_v<Tags, ReadClass<ResourceManager>> || ... || std::is_same_v<Tags, WriteClass<ResourceManager>> || ...), \
                "Access Denied: Resource not registered as Writeable!");
#define FE_USE_RM_NO_CONST \
  static_assert((std::is_same_v<Tags, WriteClass<ResourceManager>> || ...), "Access Denied: Resource not registered as Writeable!");

#define FE_USE_WORLD_CONST                                                                                                 \
  static_assert((std::is_same_v<Tags, ReadClass<WorldBase>> || ... || std::is_same_v<Tags, WriteClass<WorldBase>> || ...), \
                "Access Denied: Resource not registered as Writeable!");
#define FE_USE_WORLD_NO_CONST \
  static_assert((std::is_same_v<Tags, WriteClass<WorldBase>> || ...), "Access Denied: Resource not registered as Writeable!");

#define FE_ACCESS_COMPONENTS_READ(Components)                                                                                                 \
  static_assert(((std::is_same_v<Tags, ReadComponent<Components>> || ... || std::is_same_v<Tags, WriteComponent<Components>> || ...) && ...), \
                "Access Denied: One or more components not registered as Readable!");
#define FE_ACCESS_COMPONENTS_WRITE(Components)                                      \
  static_assert(((std::is_same_v<Tags, WriteComponent<Components>> || ...) && ...), \
                "Access Denied: One or more components not registered as Readable!");

template <typename... Tags>
struct Query {
  Query() {}

  static std::vector<PassMutex> getDependencies() { return {Tags::to_mutex()...}; }

  FE_FINLINE bool hasResource(ResourceId id) const {
    FE_USE_RM_CONST
    return m_worldBase->resourceManager()->hasResource(id);
  }

  FE_FINLINE Resource* getResource(ResourceId id) {
    FE_USE_RM_NO_CONST
    return m_worldBase->resourceManager()->getResource(id);
  }
  FE_FINLINE const Resource* getResource(ResourceId id) const {
    FE_USE_RM_CONST
    return m_worldBase->resourceManager()->getResource(id);
  }

  FE_FINLINE ResourceId addResource(Resource&& res) {
    FE_USE_RM_NO_CONST
    return m_worldBase->resourceManager()->addResource(res);
  }
  FE_FINLINE bool removeResource(ResourceId id) {
    FE_USE_RM_NO_CONST
    return m_worldBase->resourceManager()->removeResource(id);
  }

  template <typename T>
  FE_FINLINE void setTypeResourceId(ResourceId id) {
    FE_USE_RM_NO_CONST
    return m_worldBase->resourceManager()->setTypeResourceId<T>(id);
  }

  template <typename T>
  FE_FINLINE ResourceId findTypeResourceId() const {
    FE_USE_RM_CONST
    return m_worldBase->resourceManager()->findTypeResourceId<T>();
  }

  FE_FINLINE void setStringResourceId(const stl::string& str, ResourceId id) {
    FE_USE_RM_NO_CONST
    return m_worldBase->resourceManager()->setStringResourceId(str, id);
  }

  FE_FINLINE ResourceId findStringResourceId(const stl::string& str) const {
    FE_USE_RM_CONST
    return m_worldBase->resourceManager()->findStringResourceId(str);
  }

  FE_FINLINE ResourceCommandBuffer* getResCommandBuffer() { return &m_resCommandBuffer; }
  FE_FINLINE void submitResCommdBuffer() {
    FE_USE_RM_NO_CONST
    m_worldBase->resourceManager()->submit(std::move(m_resCommandBuffer));
  }

  FE_FINLINE void flushResourceManager() {
    FE_USE_RM_NO_CONST
    m_worldBase->resourceManager()->flush();
  }

  FE_FINLINE bool hasEntity(Entity e) const {
    FE_USE_WORLD_CONST
    return m_worldBase->hasEntity(e);
  }
  FE_FINLINE Entity createEntity() {
    FE_USE_WORLD_NO_CONST
    return m_worldBase->createEntity();
  }

  FE_FINLINE void destroyEntity(Entity e) {
    FE_USE_WORLD_NO_CONST
    m_worldBase->destroyEntity(e);
  }
  FE_FINLINE void destroyEntityDelayed(Entity e) {
    FE_USE_WORLD_NO_CONST
    m_worldBase->destroyEntityDelayed(e);
  }

  template <typename T>
  FE_FINLINE bool hasComponents(Entity e) const {
    FE_ACCESS_COMPONENTS_READ(T)
    m_worldBase->hasComponents<T>(e);
  }

  template <typename T>
  FE_FINLINE T& getComponent(Entity e) {
    FE_ACCESS_COMPONENTS_WRITE(T)
    m_worldBase->getComponent<T>(e);
  }

  template <typename T>
  FE_FINLINE const T& getComponent(Entity e) const {
    FE_ACCESS_COMPONENTS_READ(T)
    m_worldBase->getComponent<T>(e);
  }

  template <typename T, typename... Args>
  FE_FINLINE void addComponent(Entity e, Args&&... args) {
    FE_ACCESS_COMPONENTS_WRITE(T)
    m_worldBase->addComponent<T>(e, std::forward<Args>(args)...);
  }

  template <typename T, typename... Args>
  FE_FINLINE void addComponentDelayed(Entity e, Args&&... args) {
    FE_ACCESS_COMPONENTS_WRITE(T)
    m_worldBase->addComponentDelayed<T>(e, std::forward<Args>(args)...);
  }

  template <typename T, typename... Args>
  FE_FINLINE void changeComponent(Entity e, Args&&... args) {
    FE_ACCESS_COMPONENTS_WRITE(T)
    m_worldBase->changeComponent<T>(e, std::forward<Args>(args)...);
  }

  template <typename T, typename... Args>
  FE_FINLINE void changeComponentDelayed(Entity e, Args&&... args) {
    FE_ACCESS_COMPONENTS_WRITE(T)
    m_worldBase->changeComponentDelayed<T>(e, std::forward<Args>(args)...);
  }

  template <typename T>
  FE_FINLINE void removeComponent(Entity e) {
    FE_ACCESS_COMPONENTS_WRITE(T)
    m_worldBase->removeComponent<T>(e);
  }

  template <typename T>
  FE_FINLINE void removeComponentDelayed(Entity e) {
    FE_ACCESS_COMPONENTS_WRITE(T)
    m_worldBase->removeComponentDelayed<T>(e);
  }

  template <typename... Components>
  auto view() {
    FE_ACCESS_COMPONENTS_WRITE(Components)
    return m_worldBase->view<Components...>();
  }

  template <typename... Components>
  auto view() const {
    FE_ACCESS_COMPONENTS_READ(Component)
    return m_worldBase->view<Components...>();
  }

 private:
  WorldBase* m_worldBase;
  ResourceCommandBuffer m_resCommandBuffer;
};

}  // namespace fe::engine::ecs