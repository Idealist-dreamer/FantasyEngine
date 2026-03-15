#pragma once

#include "access.h"
#include "native_type.h"

namespace fe::engine {

// ============================================================================
// Entity 访问适配
// ============================================================================
template <>
struct AccessTraits<EntityQuery> {
  static void declare(AccessInfo& acc) {
    acc.child<Registry>().child<Entity>().read();
  }
  static void prepare(Blackboard& bb, uint32_t) {
    if (!bb.has<Registry>()) bb.emplace_or_replace<Registry>();
  }
  static EntityQuery fetch(Blackboard& bb, uint32_t) {
    return EntityQuery(bb.get<Registry>());
  }
};

template <>
struct AccessTraits<EntityCreator> {
  static void declare(AccessInfo& acc) {
    acc.child<Registry>().child<Entity>().write();
  }
  static void prepare(Blackboard& bb, uint32_t) {
    if (!bb.has<Registry>()) bb.emplace_or_replace<Registry>();
  }
  static EntityCreator fetch(Blackboard& bb, uint32_t) {
    return EntityCreator(bb.get<Registry>());
  }
};

template <>
struct AccessTraits<EntityDestroyer> {
  static void declare(AccessInfo& acc) { acc.child<Registry>().write(); }
  static void prepare(Blackboard& bb, uint32_t) {
    if (!bb.has<Registry>()) bb.emplace_or_replace<Registry>();
  }
  static EntityDestroyer fetch(Blackboard& bb, uint32_t) {
    return EntityDestroyer(bb.get<Registry>());
  }
};

// ============================================================================
// Component 访问适配
// ============================================================================
template <typename... Cs>
struct AccessTraits<ComponentReader<Cs...>> {
  static void declare(AccessInfo& acc) {
    acc.child<Registry>().read();
    (acc.child<Registry>().template child<std::remove_const_t<Cs>>().read(),
     ...);
  }
  static void prepare(Blackboard& bb, uint32_t) {
    if (!bb.has<Registry>()) bb.emplace_or_replace<Registry>();
    auto& reg = bb.get<Registry>();
    // 预分配组件 storage
    (reg.template storage<std::remove_const_t<Cs>>(), ...);
    // 预分配标记组件 storage
    (reg.template storage<AddTag<std::remove_const_t<Cs>>>(), ...);
    (reg.template storage<ChangeTag<std::remove_const_t<Cs>>>(), ...);
    (reg.template storage<RemoveTag<std::remove_const_t<Cs>>>(), ...);
  }
  static ComponentReader<Cs...> fetch(Blackboard& bb, uint32_t) {
    return ComponentReader<Cs...>(bb.get<Registry>());
  }
};

template <typename... Cs>
struct AccessTraits<ComponentWriter<Cs...>> {
  static void declare(AccessInfo& acc) {
    acc.child<Registry>().read();
    (acc.child<Registry>().template child<std::remove_const_t<Cs>>().write(),
     ...);
  }
  static void prepare(Blackboard& bb, uint32_t pass_id) {
    // 继承 Reader 的预分配
    AccessTraits<ComponentReader<Cs...>>::prepare(bb, pass_id);
    auto& reg = bb.get<Registry>();
    // 预分配延迟组件 storage
    (reg.template storage<AddDelayed<std::remove_const_t<Cs>>>(), ...);
    (reg.template storage<ChangeDelayed<std::remove_const_t<Cs>>>(), ...);
    (reg.template storage<RemoveDelayed<std::remove_const_t<Cs>>>(), ...);
  }
  static ComponentWriter<Cs...> fetch(Blackboard& bb, uint32_t) {
    return ComponentWriter<Cs...>(bb.get<Registry>());
  }
};

// ============================================================================
// Event 访问适配
// ============================================================================
struct ReadEventTag {};
struct WriteEventTag {};

template <typename T>
struct EventStorage {
  stl::vector<T> current;
  stl::vector<T> next;
};

template <typename T>
struct AccessTraits<EventReader<T>> {
  static void declare(AccessInfo& acc) {
    acc.child<EventStorage<T>>().template child<ReadEventTag>().read();
  }
  static void prepare(Blackboard& bb, uint32_t) {
    if (!bb.has<EventStorage<T>>()) {
      bb.emplace_or_replace<EventStorage<T>>();
      // 注册事件交换回调到 CleanupRegistry
      if (!bb.has<CleanupRegistry>()) {
        bb.emplace_or_replace<CleanupRegistry>();
      }
      bb.get<CleanupRegistry>().register_cleanup([](Blackboard& b) {
        auto& storage = b.get<EventStorage<T>>();
        storage.current.swap(storage.next);
        storage.next.clear();
      });
    }
  }
  static EventReader<T> fetch(Blackboard& bb, uint32_t) {
    return EventReader<T>(bb.get<EventStorage<T>>().current);
  }
};

template <typename T>
struct AccessTraits<EventWriter<T>> {
  static void declare(AccessInfo& acc) {
    acc.child<EventStorage<T>>().template child<WriteEventTag>().write();
  }
  static void prepare(Blackboard& bb, uint32_t pass_id) {
    AccessTraits<EventReader<T>>::prepare(bb, pass_id);
  }
  static EventWriter<T> fetch(Blackboard& bb, uint32_t) {
    return EventWriter<T>(bb.get<EventStorage<T>>().next);
  }
};

// ============================================================================
// Context 访问适配
// ============================================================================
template <typename T>
struct ContextStorage {
  Any data;

  // 序列化支持：直接序列化内部数据 T
  template <class Archive>
  void save(Archive& ar) const {
    bool has_data = data.valid();
    ar(has_data);
    if (has_data) {
      ar(*data.get<T>());
    }
  }

  template <class Archive>
  void load(Archive& ar) {
    bool has_data;
    ar(has_data);
    if (has_data) {
      T value;
      ar(value);
      data = Any::create<T>(std::move(value));
    }
  }
};

template <typename T>
struct AccessTraits<ContextReader<T>> {
  static void declare(AccessInfo& acc) {
    acc.child<ContextStorage<T>>().read();
  }
  static void prepare(Blackboard& bb, uint32_t) {
    if (!bb.has<ContextStorage<T>>())
      bb.emplace_or_replace<ContextStorage<T>>();
  }
  static ContextReader<T> fetch(Blackboard& bb, uint32_t) {
    return ContextReader<T>(bb.get<ContextStorage<T>>().data);
  }
};

template <typename T>
struct AccessTraits<ContextWriter<T>> {
  static void declare(AccessInfo& acc) {
    acc.child<ContextStorage<T>>().write();
  }
  static void prepare(Blackboard& bb, uint32_t pass_id) {
    AccessTraits<ContextReader<T>>::prepare(bb, pass_id);
  }
  static ContextWriter<T> fetch(Blackboard& bb, uint32_t) {
    return ContextWriter<T>(bb.get<ContextStorage<T>>().data);
  }
};

}  // namespace fe::engine
