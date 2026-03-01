#pragma once

#include "engine/base/pch.h"

namespace fe::engine::ecs {
enum class MutexType : uint8_t {
  None = 0,
  Exclusive,
  EntityQuery,
  EntityCreate,
  EntityDestroy,
  ComponentRead,
  ComponentReadWrite,
  ClassUseConst,
  ClassUseNoConst
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
      return otherSide == MutexType::EntityDestroy || otherSide == MutexType::ComponentRead || otherSide == MutexType::ComponentReadWrite;
    }

    if (checkOneOther(m_type, other.m_type, MutexType::ComponentRead, otherSide)) {
      return otherSide == MutexType::ComponentReadWrite && m_tag == other.m_tag;
    }

    if (checkOneOther(m_type, other.m_type, MutexType::ComponentReadWrite, otherSide)) {
      return otherSide == MutexType::ComponentReadWrite && m_tag == other.m_tag;
    }

    if (checkOneOther(m_type, other.m_type, MutexType::ClassUseConst, otherSide)) {
      return otherSide == MutexType::ClassUseNoConst && m_tag == other.m_tag;
    }

    if (checkOneOther(m_type, other.m_type, MutexType::ClassUseNoConst, otherSide)) {
      return otherSide == MutexType::ClassUseNoConst && m_tag == other.m_tag;
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
  static Mutex use_class_const();

  template <typename T>
  static Mutex use_class_no_const();
};

}  // namespace fe::engine::ecs

// Impl
namespace fe::engine::ecs {
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
  Mutex mutex(MutexType::ComponentReadWrite);
  mutex.m_tag = typeid(T).hash_code();
  return mutex;
}

template <typename T>
Mutex Mutex::use_class_const() {
  Mutex mutex(MutexType::ClassUseConst);
  mutex.m_tag = typeid(T).hash_code();
  return mutex;
}

template <typename T>
Mutex Mutex::use_class_no_const() {
  Mutex mutex(MutexType::ClassUseNoConst);
  mutex.m_tag = typeid(T).hash_code();
  return mutex;
}

}  // namespace fe::engine::ecs