#pragma once

#include <entt/entt.hpp>
#include "context_adapter.h"

namespace fe::engine {

using Entity = entt::entity;
using Registry = entt::registry;

// ============================================================================
// 1. Entity & Component 访问抽象
// ============================================================================
template <typename... Components>
class ComponentReader {
 public:
  explicit ComponentReader(Registry* reg) : m_reg(reg) {}
  auto view() const { return m_reg->view<const Components...>(); }

 private:
  Registry* m_reg;
};

template <typename... Components>
class ComponentWriter {
 public:
  explicit ComponentWriter(Registry* reg) : m_reg(reg) {}
  auto view() { return m_reg->view<Components...>(); }

 private:
  Registry* m_reg;
};

template <typename... Cs>
struct ContextAdapter<ComponentReader<Cs...>> {
  static void prepare(SceneContext& sc, uint32_t) {
    if (!sc.has_context<Registry>())
      sc.emplace<Registry>();
  }
  static ContextMutex get_mutex() {
    ContextMutex m;
    (m.add_read<Registry, Cs>(), ...);
    return m;
  }
  static auto bind(SceneContext& sc, uint32_t) { return ComponentReader<Cs...>(sc.get<Registry>()); }
};

template <typename... Cs>
struct ContextAdapter<ComponentWriter<Cs...>> {
  static void prepare(SceneContext& sc, uint32_t) {
    if (!sc.has_context<Registry>())
      sc.emplace<Registry>();
  }
  static ContextMutex get_mutex() {
    ContextMutex m;
    (m.add_write<Registry, Cs>(), ...);
    return m;
  }
  static auto bind(SceneContext& sc, uint32_t) { return ComponentWriter<Cs...>(sc.get<Registry>()); }
};

// ============================================================================
// 2. Event 双缓冲系统
// ============================================================================
template <typename T>
struct EventBuffer {
  stl::vector<T> buffers[2];
  int read_idx = 0;
};

template <typename T>
class EventReader {
 public:
  explicit EventReader(const stl::vector<T>* events) : m_events(events) {}
  const stl::vector<T>& get() const { return *m_events; }

 private:
  const stl::vector<T>* m_events;
};

template <typename T>
class EventWriter {
 public:
  explicit EventWriter(stl::vector<T>* events) : m_events(events) {}
  void push(T event) { m_events->push_back(std::move(event)); }

 private:
  stl::vector<T>* m_events;
};

template <typename T>
struct ContextAdapter<EventReader<T>> {
  static void prepare(SceneContext& sc, uint32_t) {
    if (!sc.has_context<EventBuffer<T>>()) {
      auto* buf = sc.emplace<EventBuffer<T>>();
      sc.add_refresh([buf]() {
        buf->buffers[buf->read_idx].clear();  // 清空刚被读完的
        buf->read_idx = 1 - buf->read_idx;    // 交换读写指针
      });
    }
  }
  static ContextMutex get_mutex() {
    ContextMutex m;
    m.add_read<EventBuffer<T>, EventBuffer<T>>();
    return m;
  }
  static auto bind(SceneContext& sc, uint32_t) {
    auto* buf = sc.get<EventBuffer<T>>();
    return EventReader<T>(&buf->buffers[buf->read_idx]);
  }
};

template <typename T>
struct ContextAdapter<EventWriter<T>> {
  static void prepare(SceneContext& sc, uint32_t id) { ContextAdapter<EventReader<T>>::prepare(sc, id); }
  static ContextMutex get_mutex() {
    ContextMutex m;
    m.add_write<EventBuffer<T>, EventBuffer<T>>();
    return m;
  }
  static auto bind(SceneContext& sc, uint32_t) {
    auto* buf = sc.get<EventBuffer<T>>();
    return EventWriter<T>(&buf->buffers[1 - buf->read_idx]);
  }
};

// ============================================================================
// 3. 全局 Context 访问
// ============================================================================
template <typename T>
class ContextRes {
 public:
  explicit ContextRes(T* ptr) : m_ptr(ptr) {}
  T& get() { return *m_ptr; }

 private:
  T* m_ptr;
};

template <typename T>
struct ContextAdapter<ContextRes<T>> {
  static void prepare(SceneContext& sc, uint32_t) {
    if (!sc.has_context<T>())
      sc.emplace<T>();
  }
  static ContextMutex get_mutex() {
    ContextMutex m;
    m.add_write<T, T>();
    return m;
  }
  static auto bind(SceneContext& sc, uint32_t) { return ContextRes<T>(sc.get<T>()); }
};

// ============================================================================
// 4. EntityCommandBuffer (ECB) 完美解耦
// ============================================================================
// 每个 Pass 分配独立的队列，避免多线程锁。刷新时统一 Play 到 Registry。
struct ECBManager {
  stl::unordered_map<uint32_t, stl::vector<std::function<void(Registry&)>>> pass_queues;
};

class EntityCommandBuffer {
 public:
  explicit EntityCommandBuffer(stl::vector<std::function<void(Registry&)>>* queue) : m_queue(queue) {}

  template <typename T>
  void add_component(Entity e, T comp) {
    m_queue->push_back([e, c = std::move(comp)](Registry& r) { r.emplace<T>(e, c); });
  }

 private:
  stl::vector<std::function<void(Registry&)>>* m_queue;
};

template <>
struct ContextAdapter<EntityCommandBuffer> {
  static void prepare(SceneContext& sc, uint32_t pass_id) {
    if (!sc.has_context<ECBManager>()) {
      auto* mgr = sc.emplace<ECBManager>();
      sc.add_refresh([sc_ptr = &sc]() {
        auto* manager = sc_ptr->get<ECBManager>();
        auto* reg = sc_ptr->get<Registry>();
        for (auto& [pid, queue] : manager->pass_queues) {
          for (auto& cmd : queue)
            cmd(*reg);
          queue.clear();
        }
      });
    }
  }
  static ContextMutex get_mutex() { return ContextMutex{}; }  // 写入私有队列，完全不冲突！
  static auto bind(SceneContext& sc, uint32_t pass_id) {
    auto* mgr = sc.get<ECBManager>();
    return EntityCommandBuffer(&mgr->pass_queues[pass_id]);
  }
};

}  // namespace fe::engine