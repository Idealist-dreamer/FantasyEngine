#pragma once

#include "access.h"
#include "blackboard.h"
#include "paramTypes.h"

namespace fe::engine {

struct EntityStructureAccess {};  // 用于保护实体创建/销毁的并发写冲突

template <typename T>
struct ParamProvider {
  static void build_access(Access& acc) { acc.child<T>().write(); }
  static void prepare(Blackboard& bb) {
    if (!bb.has<T>()) bb.emplace<T>();
  }
  static T& fetch(Blackboard& bb) { return bb.get<T>(); }
};

// ==========================================
// 1. Entity 操作适配
// ==========================================
template <>
struct ParamProvider<EntityQuery> {
  static void build_access(Access& acc) {
    acc.child<entt::registry>().read();
    acc.child<EntityStructureAccess>().read();
  }
  static void prepare(Blackboard& bb) {
    if (!bb.has<entt::registry>()) bb.emplace<entt::registry>();
  }
  static EntityQuery fetch(Blackboard& bb) {
    return EntityQuery(bb.get<entt::registry>());
  }
};

template <>
struct ParamProvider<EntityCreator> {
  static void build_access(Access& acc) {
    acc.child<entt::registry>().read();
    acc.child<EntityStructureAccess>().write();
  }
  static void prepare(Blackboard& bb) {
    if (!bb.has<entt::registry>()) bb.emplace<entt::registry>();
  }
  static EntityCreator fetch(Blackboard& bb) {
    return EntityCreator(bb.get<entt::registry>());
  }
};

template <>
struct ParamProvider<EntityDestroyer> {
  static void build_access(Access& acc) {
    acc.child<entt::registry>().read();
    acc.child<EntityStructureAccess>().write();
  }
  static void prepare(Blackboard& bb) {
    if (!bb.has<entt::registry>()) bb.emplace<entt::registry>();
  }
  static EntityDestroyer fetch(Blackboard& bb) {
    return EntityDestroyer(bb.get<entt::registry>());
  }
};

template <>
struct ParamProvider<EntityCommandBuffer> {
  static void build_access(Access& acc) {
    acc.child<EntityCommandBuffer>().write();
  }
  static void prepare(Blackboard& bb) {
    if (!bb.has<EntityCommandBuffer>()) {
      bb.emplace<EntityCommandBuffer>();
      bb.add_cleanup([&bb]() {
        auto& ecb = bb.get<EntityCommandBuffer>();
        auto& reg = bb.get<entt::registry>();
        for (auto& entity : ecb.m_entity_map) {
          if (entity == entt::null) entity = reg.create();
        }
        for (auto& e : ecb.m_destroyed_entities) {
          reg.destroy(e);
        }
        ecb.clear();
      });
    }
  }
  static decltype(auto) fetch(Blackboard& bb) {
    return std::ref(bb.get<EntityCommandBuffer>());
  }
};

// ==========================================
// 2. Component 读写适配
// ==========================================
template <typename... Cs>
struct ParamProvider<ComponentReader<Cs...>> {
  static void build_access(Access& acc) {
    auto& reg_acc = acc.child<entt::registry>().read();
    (reg_acc.child<std::remove_const_t<Cs>>().read(), ...);
  }
  static void prepare(Blackboard& bb) {
    if (!bb.has<entt::registry>()) bb.emplace<entt::registry>();
    auto& reg = bb.get<entt::registry>();
    (reg.template storage<std::remove_const_t<Cs>>(), ...);
  }
  static ComponentReader<Cs...> fetch(Blackboard& bb) {
    return ComponentReader<Cs...>(bb.get<entt::registry>());
  }
};

template <typename... Cs>
struct ParamProvider<ComponentWriter<Cs...>> {
  static void build_access(Access& acc) {
    auto& reg_acc = acc.child<entt::registry>().read();
    (reg_acc.child<std::remove_const_t<Cs>>().write(), ...);
  }
  static void prepare(Blackboard& bb) {
    if (!bb.has<entt::registry>()) bb.emplace<entt::registry>();
    auto& reg = bb.get<entt::registry>();
    (reg.template storage<std::remove_const_t<Cs>>(), ...);
  }
  static ComponentWriter<Cs...> fetch(Blackboard& bb) {
    return ComponentWriter<Cs...>(bb.get<entt::registry>());
  }
};

// ==========================================
// 3. Event 双缓冲适配
// ==========================================
template <typename T>
struct DoubleBufferEvent {
  stl::vector<T> read_buffer;
  stl::vector<T> write_buffer;
  void swap() {
    read_buffer.swap(write_buffer);
    write_buffer.clear();
  }
};

template <typename T>
struct ParamProvider<EventReader<T>> {
  static void build_access(Access& acc) {
    acc.child<DoubleBufferEvent<T>>().read();
  }
  static void prepare(Blackboard& bb) {
    if (!bb.has<DoubleBufferEvent<T>>()) {
      bb.emplace<DoubleBufferEvent<T>>();
      bb.add_cleanup([&bb]() { bb.get<DoubleBufferEvent<T>>().swap(); });
    }
  }
  static EventReader<T> fetch(Blackboard& bb) {
    return EventReader<T>(bb.get<DoubleBufferEvent<T>>().read_buffer);
  }
};

template <typename T>
struct ParamProvider<EventWriter<T>> {
  static void build_access(Access& acc) {
    acc.child<DoubleBufferEvent<T>>().write();
  }
  static void prepare(Blackboard& bb) {
    if (!bb.has<DoubleBufferEvent<T>>()) {
      bb.emplace<DoubleBufferEvent<T>>();
      bb.add_cleanup([&bb]() { bb.get<DoubleBufferEvent<T>>().swap(); });
    }
  }
  static EventWriter<T> fetch(Blackboard& bb) {
    return EventWriter<T>(bb.get<DoubleBufferEvent<T>>().write_buffer);
  }
};

// ==========================================
// 4. Context 适配
// ==========================================
template <typename T>
struct ParamProvider<ContextReader<T>> {
  static void build_access(Access& acc) { acc.child<T>().read(); }
  static void prepare(Blackboard& bb) {
    if (!bb.has<Any>()) bb.emplace<Any>();
  }  // Simplified for Context
  static Context<T> fetch(Blackboard& bb) { return Context<T>(bb.get<Any>()); }
};

template <typename T>
struct ParamProvider<ContextWriter<T>> {
  static void build_access(Access& acc) { acc.child<T>().write(); }
  static void prepare(Blackboard& bb) {
    if (!bb.has<Any>()) bb.emplace<Any>();
  }
  static Context<T> fetch(Blackboard& bb) { return Context<T>(bb.get<Any>()); }
};

// ==========================================
// 元编程聚合辅助
// ==========================================
struct ParamOps {
  template <typename... Args>
  static Access build_merged_access() {
    Access root_acc;
    (ParamProvider<meta::clean_t<Args>>::build_access(root_acc), ...);
    root_acc.bake();
    return root_acc;
  }

  template <typename... Args>
  static void prepare_all(Blackboard& bb) {
    (ParamProvider<meta::clean_t<Args>>::prepare(bb), ...);
  }

  template <typename... Args>
  static auto fetch_all(Blackboard& bb) {
    return std::make_tuple(ParamProvider<meta::clean_t<Args>>::fetch(bb)...);
  }
};

}  // namespace fe::engine