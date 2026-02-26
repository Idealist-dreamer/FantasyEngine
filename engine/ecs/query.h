#pragma once

#include "worldBase.h"

namespace fe::engine::ecs {

// --- 权限标签定义 ---
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
  static constexpr PassMutex to_mutex() { return PassMutex(MutexType::ComponentAccess, AT, typeid(T).hash_code()); }
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

// --- 静态检查宏 (针对 World 和 ResourceManager) ---
#define FE_USE_RM_CONST                                                                                                                  \
  static_assert((std::is_same_v<Tags, ReadClass<ResourceManager>> || ...) || (std::is_same_v<Tags, WriteClass<ResourceManager>> || ...), \
                "Access Denied: ResourceManager not registered!");

#define FE_USE_RM_NO_CONST \
  static_assert((std::is_same_v<Tags, WriteClass<ResourceManager>> || ...), "Access Denied: ResourceManager not registered as Writeable!");

#define FE_USE_WORLD_CONST                                                                                                   \
  static_assert((std::is_same_v<Tags, ReadClass<WorldBase>> || ...) || (std::is_same_v<Tags, WriteClass<WorldBase>> || ...), \
                "Access Denied: WorldBase not registered!");

#define FE_USE_WORLD_NO_CONST \
  static_assert((std::is_same_v<Tags, WriteClass<WorldBase>> || ...), "Access Denied: WorldBase not registered as Writeable!");

// --- Query 类定义 ---
template <typename... Tags>
struct Query {
  Query() = default;

  FE_FINLINE void setWorld(WorldBase* worldBase) { m_worldBase = worldBase; }

  static stl::vector<PassMutex> getDependencies() { return {Tags::to_mutex()...}; }

  // 内部辅助判断权限 (核心修复：利用类模板的 Tags 参数包)
  template <typename C>
  static constexpr bool has_read_perm = (std::is_same_v<Tags, ReadComponent<C>> || ...) || (std::is_same_v<Tags, WriteComponent<C>> || ...);

  template <typename C>
  static constexpr bool has_write_perm = (std::is_same_v<Tags, WriteComponent<C>> || ...);

  // --- Resource 成员函数 ---
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
    return m_worldBase->resourceManager()->addResource(std::move(res));
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

  // --- Entity 成员函数 ---
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

  // --- Component 成员函数 ---
  template <typename T>
  FE_FINLINE bool hasComponents(Entity e) const {
    static_assert(has_read_perm<T>, "Access Denied: Component not registered as Readable!");
    return m_worldBase->hasComponents<T>(e);
  }

  template <typename T>
  FE_FINLINE T& getComponent(Entity e) {
    static_assert(has_write_perm<T>, "Access Denied: Component not registered as Writeable!");
    return m_worldBase->getComponent<T>(e);
  }

  template <typename T>
  FE_FINLINE const T& getComponent(Entity e) const {
    static_assert(has_read_perm<T>, "Access Denied: Component not registered as Readable!");
    return m_worldBase->getComponentConst<T>(e);
  }

  template <typename T, typename... Args>
  FE_FINLINE void addComponent(Entity e, Args&&... args) {
    static_assert(has_write_perm<T>, "Access Denied: Component not registered as Writeable!");
    m_worldBase->addComponent<T>(e, std::forward<Args>(args)...);
  }

  template <typename T, typename... Args>
  FE_FINLINE void addComponentDelayed(Entity e, Args&&... args) {
    static_assert(has_write_perm<T>, "Access Denied: Component not registered as Writeable!");
    m_worldBase->addComponentDelayed<T>(e, std::forward<Args>(args)...);
  }

  template <typename T, typename... Args>
  FE_FINLINE void changeComponent(Entity e, Args&&... args) {
    static_assert(has_write_perm<T>, "Access Denied: Component not registered as Writeable!");
    m_worldBase->changeComponent<T>(e, std::forward<Args>(args)...);
  }

  template <typename T, typename... Args>
  FE_FINLINE void changeComponentDelayed(Entity e, Args&&... args) {
    static_assert(has_write_perm<T>, "Access Denied: Component not registered as Writeable!");
    m_worldBase->changeComponentDelayed<T>(e, std::forward<Args>(args)...);
  }

  template <typename T>
  FE_FINLINE void removeComponent(Entity e) {
    static_assert(has_write_perm<T>, "Access Denied: Component not registered as Writeable!");
    m_worldBase->removeComponent<T>(e);
  }

  template <typename T>
  FE_FINLINE void removeComponentDelayed(Entity e) {
    static_assert(has_write_perm<T>, "Access Denied: Component not registered as Writeable!");
    m_worldBase->removeComponentDelayed<T>(e);
  }

  // --- View 成员函数 (处理参数包) ---
  template <typename... Components>
  auto view() {
    static_assert((has_write_perm<Components> && ...), "Access Denied: One or more components not registered as Writeable!");
    return m_worldBase->view<Components...>();
  }

  template <typename... Components>
  auto viewConst() const {
    static_assert((has_read_perm<Components> && ...), "Access Denied: One or more components not registered as Readable!");
    return m_worldBase->viewConst<Components...>();
  }

 private:
  WorldBase* m_worldBase = nullptr;
  ResourceCommandBuffer m_resCommandBuffer;
};

}  // namespace fe::engine::ecs