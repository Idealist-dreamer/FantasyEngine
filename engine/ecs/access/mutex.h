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

template <typename T>
FE_FINLINE bool checkOneOther(T t1, T t2, T x, T& other) {
  if (t1 == x) {
    other = t2;
    return true;
  }
  if (t2 == x) {
    other = t1;
    return true;
  }
  return false;
}

struct Mutex {
  Mutex(MutexType _type = MutexType::None, size_t _tag = 0) : type(_type), tag(_tag) {}

  MutexType type;
  size_t tag;

  auto operator<=>(const Mutex& other) const = default;
  bool operator==(const Mutex& other) const { return (*this <=> other) == 0; }

  bool isConflict(const Mutex& other) const {
    if (type == MutexType::Exclusive || other.type == MutexType::Exclusive) {
      return true;
    }

    MutexType otherSide;

    if (checkOneOther(type, other.type, MutexType::EntityQuery, otherSide)) {
      return otherSide == MutexType::EntityCreate || otherSide == MutexType::EntityDestroy;
    }

    if (checkOneOther(type, other.type, MutexType::EntityCreate, otherSide)) {
      return otherSide == MutexType::EntityCreate || otherSide == MutexType::EntityDestroy;
    }

    if (checkOneOther(type, other.type, MutexType::EntityDestroy, otherSide)) {
      return otherSide == MutexType::EntityDestroy || otherSide == MutexType::ComponentRead || otherSide == MutexType::ComponentReadWrite;
    }

    if (checkOneOther(type, other.type, MutexType::ComponentRead, otherSide)) {
      return otherSide == MutexType::ComponentReadWrite && tag == other.tag;
    }

    if (checkOneOther(type, other.type, MutexType::ComponentReadWrite, otherSide)) {
      return otherSide == MutexType::ComponentReadWrite && tag == other.tag;
    }

    if (checkOneOther(type, other.type, MutexType::ClassUseConst, otherSide)) {
      return otherSide == MutexType::ClassUseNoConst && tag == other.tag;
    }

    if (checkOneOther(type, other.type, MutexType::ClassUseNoConst, otherSide)) {
      return otherSide == MutexType::ClassUseNoConst && tag == other.tag;
    }

    return false;
  }

  FE_FINLINE static Mutex QueryEntity();
  FE_FINLINE static Mutex CreateEntity();
  FE_FINLINE static Mutex DestroyEntity();

  template <typename T>
  static Mutex ReadComponent();

  template <typename T>
  static Mutex WriteComponent();

  template <typename T>
  static Mutex UseClassConst();

  template <typename T>
  static Mutex UseClassNoConst();
};

}  // namespace fe::engine::ecs

// Impl
namespace fe::engine::ecs {
FE_FINLINE Mutex Mutex::QueryEntity() {
  Mutex mutex(MutexType::EntityQuery);
  return mutex;
}
FE_FINLINE Mutex Mutex::CreateEntity() {
  Mutex mutex(MutexType::EntityCreate);
  return mutex;
}
FE_FINLINE Mutex Mutex::DestroyEntity() {
  Mutex mutex(MutexType::EntityDestroy);
  return mutex;
}

template <typename T>
Mutex Mutex::ReadComponent() {
  Mutex mutex(MutexType::ComponentRead);
  mutex.tag = typeid(T).hash_code();
  return mutex;
}

template <typename T>
Mutex Mutex::WriteComponent() {
  Mutex mutex(MutexType::ComponentReadWrite);
  mutex.tag = typeid(T).hash_code();
  return mutex;
}

template <typename T>
Mutex Mutex::UseClassConst() {
  Mutex mutex(MutexType::ClassUseConst);
  mutex.tag = typeid(T).hash_code();
  return mutex;
}

template <typename T>
Mutex Mutex::UseClassNoConst() {
  Mutex mutex(MutexType::ClassUseNoConst);
  mutex.tag = typeid(T).hash_code();
  return mutex;
}

}  // namespace fe::engine::ecs