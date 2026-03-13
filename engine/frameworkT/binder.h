#pragma once

#include "access.h"
#include "blackboard.h"
#include "paramTypes.h"

namespace fe::engine {

// ============================================================================
// ParamProvider: Unified parameter handling protocol
// Each parameter type declares its own access, preparation, and fetch logic.
// ============================================================================

// Default provider for arbitrary types
template <typename T>
struct ParamProvider {
  static void build_access(Access& acc) { acc.child<T>().write(); }

  static void prepare(Blackboard& bb) {
    if (!bb.has<T>()) bb.emplace<T>();
  }

  static T& fetch(Blackboard& bb) { return bb.get<T>(); }
};

// ============================================================================
// Entity types
// ============================================================================

template <>
struct ParamProvider<EntityQuery> {
  static void build_access(Access& acc) { acc.child<Registry>().read(); }
  static void prepare(Blackboard& bb) { bb.get_or_emplace<Registry>(); }
  static EntityQuery fetch(Blackboard& bb) {
    return EntityQuery(bb.get<Registry>());
  }
};

template <>
struct ParamProvider<EntityCreator> {
  static void build_access(Access& acc) { acc.child<Registry>().write(); }
  static void prepare(Blackboard& bb) { bb.get_or_emplace<Registry>(); }
  static EntityCreator fetch(Blackboard& bb) {
    return EntityCreator(bb.get<Registry>());
  }
};

template <>
struct ParamProvider<EntityDestroyer> {
  static void build_access(Access& acc) {
    auto& reg = acc.child<Registry>().write();
    reg.child<Entity>().write();
  }
  static void prepare(Blackboard& bb) { bb.get_or_emplace<Registry>(); }
  static EntityDestroyer fetch(Blackboard& bb) {
    return EntityDestroyer(bb.get<Registry>());
  }
};

template <>
struct ParamProvider<EntityCommandBuffer> {
  static void build_access(Access& acc) {
    // Command buffers don't conflict - each pass has its own
  }
  static void prepare(Blackboard& bb) {
    // Will be created per-pass in fetch
  }
  static EntityCommandBuffer& fetch(Blackboard& bb) {
    return bb.get_or_emplace<EntityCommandBuffer>();
  }
};

// ============================================================================
// Component readers/writers
// ============================================================================

template <typename... Cs>
struct ParamProvider<ComponentReader<Cs...>> {
  static void build_access(Access& acc) {
    auto& reg_acc = acc.child<Registry>().read();
    (reg_acc.child<Cs>().read(), ...);
  }

  static void prepare(Blackboard& bb) {
    auto& reg = bb.get_or_emplace<Registry>();
    // Pre-create storage pools
    (reg.template storage<std::remove_const_t<Cs>>(), ...);
    (reg.template storage<AddTag<Cs>>(), ...);
    (reg.template storage<ChangeTag<Cs>>(), ...);
    (reg.template storage<RemoveTag<Cs>>(), ...);
    (reg.template storage<AddDelayed<Cs>>(), ...);
    (reg.template storage<ChangeDelayed<Cs>>(), ...);
    (reg.template storage<RemoveDelayed<Cs>>(), ...);
  }

  static ComponentReader<Cs...> fetch(Blackboard& bb) {
    return ComponentReader<Cs...>(bb.get<Registry>());
  }
};

template <typename... Cs>
struct ParamProvider<ComponentWriter<Cs...>> {
  static void build_access(Access& acc) {
    auto& reg_acc = acc.child<Registry>().read();
    (reg_acc.child<Cs>().write(), ...);
  }

  static void prepare(Blackboard& bb) {
    auto& reg = bb.get_or_emplace<Registry>();
    (reg.template storage<std::remove_const_t<Cs>>(), ...);
    (reg.template storage<AddTag<Cs>>(), ...);
    (reg.template storage<ChangeTag<Cs>>(), ...);
    (reg.template storage<RemoveTag<Cs>>(), ...);
    (reg.template storage<AddDelayed<Cs>>(), ...);
    (reg.template storage<ChangeDelayed<Cs>>(), ...);
    (reg.template storage<RemoveDelayed<Cs>>(), ...);
  }

  static ComponentWriter<Cs...> fetch(Blackboard& bb) {
    return ComponentWriter<Cs...>(bb.get<Registry>());
  }
};

// ============================================================================
// Event readers/writers (double-buffered)
// ============================================================================

template <typename T>
struct ParamProvider<EventReader<T>> {
  static void build_access(Access& acc) { acc.child<DoubleBuffer<T>>().read(); }

  static void prepare(Blackboard& bb) {
    if (!bb.has<DoubleBuffer<T>>()) {
      bb.emplace<DoubleBuffer<T>>();
      // Auto-register frame-end swap
      bb.add_cleanup([&bb]() { bb.get<DoubleBuffer<T>>().swap(); });
    }
  }

  static EventReader<T> fetch(Blackboard& bb) {
    return EventReader<T>(bb.get<DoubleBuffer<T>>().read_buf);
  }
};

template <typename T>
struct ParamProvider<EventWriter<T>> {
  static void build_access(Access& acc) {
    acc.child<DoubleBuffer<T>>().write();
  }

  static void prepare(Blackboard& bb) {
    if (!bb.has<DoubleBuffer<T>>()) {
      bb.emplace<DoubleBuffer<T>>();
      bb.add_cleanup([&bb]() { bb.get<DoubleBuffer<T>>().swap(); });
    }
  }

  static EventWriter<T> fetch(Blackboard& bb) {
    return EventWriter<T>(bb.get<DoubleBuffer<T>>().write_buf);
  }
};

// ============================================================================
// Context readers/writers
// Note: Context stores its data in Any wrapper for type-erased access
// ============================================================================

// Helper to get context storage key
template <typename T>
struct ContextStorage {
  Any data;
};

template <typename T>
struct ParamProvider<ContextReader<T>> {
  static void build_access(Access& acc) {
    acc.child<ContextStorage<T>>().read();
  }
  static void prepare(Blackboard& bb) {
    bb.get_or_emplace<ContextStorage<T>>();
  }
  static ContextReader<T> fetch(Blackboard& bb) {
    return ContextReader<T>(bb.get<ContextStorage<T>>().data);
  }
};

template <typename T>
struct ParamProvider<ContextWriter<T>> {
  static void build_access(Access& acc) {
    acc.child<ContextStorage<T>>().write();
  }
  static void prepare(Blackboard& bb) {
    bb.get_or_emplace<ContextStorage<T>>();
  }
  static ContextWriter<T> fetch(Blackboard& bb) {
    return ContextWriter<T>(bb.get<ContextStorage<T>>().data);
  }
};

// ============================================================================
// ParamOps: Aggregate operations for parameter packs
// ============================================================================

struct ParamOps {
  template <typename... Args>
  static Access build_access() {
    Access root;
    (ParamProvider<clean_t<Args>>::build_access(root), ...);
    root.bake();
    return root;
  }

  template <typename... Args>
  static void prepare(Blackboard& bb) {
    (ParamProvider<clean_t<Args>>::prepare(bb), ...);
  }

  template <typename... Args>
  static auto fetch(Blackboard& bb) {
    return std::make_tuple(ParamProvider<clean_t<Args>>::fetch(bb)...);
  }
};

}  // namespace fe::engine
