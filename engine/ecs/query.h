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
  // 修正：使用 typeid(T).hash_code()，注意 MSVC 下 constexpr 的限制
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

// --- 静态检查辅助宏 ---

// 检查资源权限
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

// --- 修正后的组件权限检查 ---
// 原理：定义内部辅助 constexpr 变量来判断单个组件权限，然后再处理参数包

template <typename... Tags>
struct Query {
  Query() = default;

  FE_FINLINE void setWorld(WorldBase* worldBase) { m_worldBase = worldBase; }

  static std::vector<PassMutex> getDependencies() { return {Tags::to_mutex()...}; }

  // 内部权限判断辅助（利用 C++17 Fold Expressions 检查 Tags 包）
  template <typename C>
  static constexpr bool has_read_perm = (std::is_same_v<Tags, ReadComponent<C>> || ...) || (std::is_same_v<Tags, WriteComponent<C>> || ...);

  template <typename C>
  static constexpr bool has_write_perm = (std::is_same_v<Tags, WriteComponent<C>> || ...);

  // --- Resource Methods ---
  FE_FINLINE bool hasResource(ResourceId id) const {
    FE_USE_RM_CONST
    return m_worldBase->resourceManager()->hasResource(id);
  }

  FE_FINLINE Resource* getResource(ResourceId id) {
    FE_USE_RM_NO_CONST
    return m_worldBase->resourceManager()->getResource(id);
  }

  FE_FINLINE ResourceId addResource(Resource&& res) {
    FE_USE_RM_NO_CONST
    return m_worldBase->resourceManager()->addResource(std::move(res));
  }

  // --- Entity Methods ---
  FE_FINLINE Entity createEntity() {
    FE_USE_WORLD_NO_CONST
    return m_worldBase->createEntity();
  }

  FE_FINLINE void destroyEntity(Entity e) {
    FE_USE_WORLD_NO_CONST
    m_worldBase->destroyEntity(e);
  }

  // --- Component Methods ---

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
    return m_worldBase->getComponent<T>(e);
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

  template <typename T>
  FE_FINLINE void removeComponent(Entity e) {
    static_assert(has_write_perm<T>, "Access Denied: Component not registered as Writeable!");
    m_worldBase->removeComponent<T>(e);
  }

  // --- View Methods (处理参数包) ---

  template <typename... Components>
  auto view() {
    // 对 Components 包里的每一个组件 C，都检查是否有写权限
    static_assert((has_write_perm<Components> && ...), "Access Denied: One or more components not registered as Writeable!");
    return m_worldBase->view<Components...>();
  }

  template <typename... Components>
  auto view() const {
    // 对 Components 包里的每一个组件 C，都检查是否有读权限
    static_assert((has_read_perm<Components> && ...), "Access Denied: One or more components not registered as Readable!");
    return m_worldBase->view<Components...>();
  }

 private:
  WorldBase* m_worldBase = nullptr;
  ResourceCommandBuffer m_resCommandBuffer;
};

}  // namespace fe::engine::ecs