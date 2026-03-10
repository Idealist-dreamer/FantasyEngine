#pragma once

#include "common.h"
#include "context.h"
#include "asset.h"
#include "external.h"

#include <typeindex>

#include "paramTypes.h"
#include "paramTraits.h"
#include "paramAccess.h"

namespace fe::engine {

/// ============================================================================
/// WorldVisitor - 非模板访问器，提供对 WorldBase 资源的安全访问
/// 职责：资源预先申请、初始化设置、运行时访问
/// ============================================================================
class WorldVisitor {
 public:
  explicit WorldVisitor(WorldBase& world) : m_world(world) {}

  WorldVisitor(const WorldVisitor&) = delete;
  WorldVisitor& operator=(const WorldVisitor&) = delete;

  // ==================== Context 访问接口 ====================

  /// 获取 Context 存储（用于创建/访问上下文资源）
  template <typename T>
  ContextStorage& get_context() {
    return m_world.m_context_manager[typeid(T)];
  }

  /// 确保 Context 存储已初始化
  template <typename T>
  void prepare_context() {
    auto tid = std::type_index(typeid(T));
    if (m_world.m_context_manager.find(tid) == m_world.m_context_manager.end()) {
      m_world.m_context_manager.insert({tid, ContextStorage()});
    }
  }

  // ==================== Event 访问接口 ====================

  /// 确保 Event 双缓冲存储已初始化
  template <typename T>
  void prepare_event() {
    auto tid = std::type_index(typeid(T));
    if (m_world.m_event_manager1.find(tid) == m_world.m_event_manager1.end()) {
      m_world.m_event_manager1.insert({tid, ContextStorage::create<stl::vector<T>>()});
      m_world.m_event_manager2.insert({tid, ContextStorage::create<stl::vector<T>>()});
      m_world.m_event_swap[tid] = [](WorldBase& wb) {
        auto inner_tid = std::type_index(typeid(T));
        auto& data1 = *(wb.m_event_manager1[inner_tid].template get<stl::vector<T>>());
        auto& data2 = *(wb.m_event_manager2[inner_tid].template get<stl::vector<T>>());
        data1.swap(data2);
        data2.clear();
      };
    }
  }

  /// 获取事件管理器（供 ParamAccess 使用）
  stl::unordered_map<std::type_index, ContextStorage>& get_event_manager1() {
    return m_world.m_event_manager1;
  }
  stl::unordered_map<std::type_index, ContextStorage>& get_event_manager2() {
    return m_world.m_event_manager2;
  }

  // ==================== EntityCommandBuffer 访问接口 ====================

  /// 确保 EntityCommandBuffer 已为指定 Pass 初始化
  void prepare_entity_command_buffer(uint32_t passId) {
    if (m_world.m_entity_command_buffers.find(passId) == m_world.m_entity_command_buffers.end()) {
      m_world.m_entity_command_buffers.insert({passId, EntityCommandBuffer()});
    }
  }

  /// 获取 EntityCommandBuffer 映射
  stl::unordered_map<uint32_t, EntityCommandBuffer>& get_entity_command_buffers() {
    return m_world.m_entity_command_buffers;
  }

  // ==================== Registry 访问接口 ====================

  /// 获取 Registry 引用
  Registry& get_registry() { return m_world.m_registry; }

  /// 预先准备组件存储
  template <typename... Components>
  void prepare_component_storage() {
    (m_world.m_registry.template storage<std::remove_const_t<Components>>(), ...);
    (m_world.m_registry.template storage<AddComponentTag<Components>>(), ...);
    (m_world.m_registry.template storage<ChangeComponentTag<Components>>(), ...);
    (m_world.m_registry.template storage<RemoveComponentTag<Components>>(), ...);
    (m_world.m_registry.template storage<AddComponentDelayed<Components>>(), ...);
    (m_world.m_registry.template storage<ChangeComponentDelayed<Components>>(), ...);
    (m_world.m_registry.template storage<RemoveComponentDelayed<Components>>(), ...);
  }

  /// 从 tuple 类型准备组件存储（用于 PreparerCollector）
  template <typename Tuple>
  void prepare_component_storage_from_tuple();

  template <typename... Cs>
  void prepare_component_storage_from_tuple_impl() {
    prepare_component_storage<Cs...>();
  }

  // ==================== 原始 WorldBase 访问（仅限特殊情况） ====================

  WorldBase& get_world() { return m_world; }

 private:
  WorldBase& m_world;
};

/// ============================================================================
/// WorldBase - ECS 世界基类
/// ============================================================================
class WorldBase {
 public:
  WorldBase() = default;
  virtual ~WorldBase() = default;

  template <typename T>
  ContextStorage& get_context() {
    return m_context_manager[typeid(T)];
  }

 protected:
  Registry m_registry;

  stl::unordered_map<std::type_index, ContextStorage> m_context_manager;

  stl::unordered_map<std::type_index, ContextStorage> m_event_manager1;
  stl::unordered_map<std::type_index, ContextStorage> m_event_manager2;
  stl::unordered_map<std::type_index, void (*)(WorldBase&)> m_event_swap;

  stl::unordered_map<uint32_t, EntityCommandBuffer> m_entity_command_buffers;

  friend class WorldVisitor;
  friend class World;
};

/// prepare_component_storage_from_tuple 的 tuple 特化实现
template <typename Tuple>
void WorldVisitor::prepare_component_storage_from_tuple() {}

template <typename... Cs>
void WorldVisitor::prepare_component_storage_from_tuple<std::tuple<Cs...>>() {
  prepare_component_storage<Cs...>();
}

}  // namespace fe::engine
