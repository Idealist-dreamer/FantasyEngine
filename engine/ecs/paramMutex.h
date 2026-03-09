#pragma once

#include "engine/base/macros.h"
#include "engine/base/container/stl.h"
#include "engine/base/utility/common.h"
#include "engine/base/utility/assert.h"

namespace fe::engine {
enum class MutexType : uint8_t {
  None = 0,
  Exclusive,
  EntityQuery,
  EntityCreate,
  EntityDestroy,
  ComponentRead,
  ComponentWrite,
  ContextRead,
  ContextWrite,
  EventRead,
  EventWrite,
};

struct Mutex {
  Mutex(MutexType _type = MutexType::None, size_t _tag = 0) : m_type(_type), m_tag(_tag) {}

  MutexType m_type;
  size_t m_tag;

  auto operator<=>(const Mutex& other) const = default;
  bool operator==(const Mutex& other) const { return (*this <=> other) == 0; }

  bool is_conflict(const Mutex& other) const {
    if (m_type == MutexType::Exclusive || other.m_type == MutexType::Exclusive) {
      return true;
    }

    MutexType otherSide;

    if (checkOneOther(m_type, other.m_type, MutexType::EntityQuery, otherSide)) {
      return otherSide == MutexType::EntityCreate || otherSide == MutexType::EntityDestroy;
    }

    if (checkOneOther(m_type, other.m_type, MutexType::EntityCreate, otherSide)) {
      return otherSide == MutexType::EntityCreate || otherSide == MutexType::EntityDestroy;
    }

    if (checkOneOther(m_type, other.m_type, MutexType::EntityDestroy, otherSide)) {
      return otherSide == MutexType::EntityDestroy || otherSide == MutexType::ComponentRead || otherSide == MutexType::ComponentWrite;
    }

    if (checkOneOther(m_type, other.m_type, MutexType::ComponentRead, otherSide)) {
      return otherSide == MutexType::ComponentWrite && m_tag == other.m_tag;
    }

    if (checkOneOther(m_type, other.m_type, MutexType::ComponentWrite, otherSide)) {
      return otherSide == MutexType::ComponentWrite && m_tag == other.m_tag;
    }

    if (checkOneOther(m_type, other.m_type, MutexType::ContextRead, otherSide)) {
      return otherSide == MutexType::ContextWrite && m_tag == other.m_tag;
    }

    if (checkOneOther(m_type, other.m_type, MutexType::ContextWrite, otherSide)) {
      return otherSide == MutexType::ContextWrite && m_tag == other.m_tag;
    }

    // Event read/write don't conflict due to double buffering:
    // EventReader reads m_event_manager1 (previous frame)
    // EventWriter writes m_event_manager2 (current frame)
    // Only EventWrite vs EventWrite needs conflict detection
    if (checkOneOther(m_type, other.m_type, MutexType::EventWrite, otherSide)) {
      return otherSide == MutexType::EventWrite && m_tag == other.m_tag;
    }

    return false;
  }

  FE_FINLINE static Mutex query_entity();
  FE_FINLINE static Mutex create_entity();
  FE_FINLINE static Mutex destroy_entity();

  template <typename T>
  static Mutex read_component();

  template <typename T>
  static Mutex write_component();

  template <typename T>
  static Mutex read_context();

  template <typename T>
  static Mutex write_context();

  template <typename T>
  static Mutex read_event();

  template <typename T>
  static Mutex write_event();
};

}  // namespace fe::engine

// Impl
namespace fe::engine {
FE_FINLINE Mutex Mutex::query_entity() {
  Mutex mutex(MutexType::EntityQuery);
  return mutex;
}
FE_FINLINE Mutex Mutex::create_entity() {
  Mutex mutex(MutexType::EntityCreate);
  return mutex;
}
FE_FINLINE Mutex Mutex::destroy_entity() {
  Mutex mutex(MutexType::EntityDestroy);
  return mutex;
}

template <typename T>
Mutex Mutex::read_component() {
  Mutex mutex(MutexType::ComponentRead);
  mutex.m_tag = typeid(T).hash_code();
  return mutex;
}

template <typename T>
Mutex Mutex::write_component() {
  Mutex mutex(MutexType::ComponentWrite);
  mutex.m_tag = typeid(T).hash_code();
  return mutex;
}

template <typename T>
Mutex Mutex::read_context() {
  Mutex mutex(MutexType::ContextRead);
  mutex.m_tag = typeid(T).hash_code();
  return mutex;
}

template <typename T>
Mutex Mutex::write_context() {
  Mutex mutex(MutexType::ContextWrite);
  mutex.m_tag = typeid(T).hash_code();
  return mutex;
}

template <typename T>
Mutex Mutex::read_event() {
  Mutex mutex(MutexType::EventRead);
  mutex.m_tag = typeid(T).hash_code();
  return mutex;
}

template <typename T>
Mutex Mutex::write_event() {
  Mutex mutex(MutexType::EventWrite);
  mutex.m_tag = typeid(T).hash_code();
  return mutex;
}

}  // namespace fe::engine