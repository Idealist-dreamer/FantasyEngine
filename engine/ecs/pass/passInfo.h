#pragma once

#include "engine/base/pch.h"

#include "engine/ecs/resource/resource.h"

namespace fe::engine::ecs {
struct PassId {
  static constexpr uint32_t Invalid = FE_UINT32_MAX;

  PassId(uint32_t _value = Invalid) : value(_value) {}

  bool null() const { return value == Invalid; }
  auto operator<=>(const PassId& other) const = default;

  uint32_t value;

  FE_FINLINE static PassId Create();

 private:
  static inline uint32_t sIdCounter;
};

enum class MutexType : uint8_t { None = 0, Exclusive, EntityAccess, ComponentAccess, UseClass };
enum class AccessType : uint8_t {
  Read = 0,
  ReadWrite,
};

struct PassMutex {
  constexpr PassMutex(MutexType _type = MutexType::None, AccessType _accessType = AccessType::Read, size_t _hashCode = 0)
      : type(_type), accessType(_accessType), hashCode(_hashCode) {}

  MutexType type;
  AccessType accessType;
  size_t hashCode;

  auto operator<=>(const PassMutex& other) const = default;
  bool operator==(const PassMutex& other) const { return (*this <=> other) == 0; }

  bool isConflict(const PassMutex& other) const {
    if (type == MutexType::Exclusive || other.type == MutexType::Exclusive) {
      return true;
    }

    if ((type == MutexType::EntityAccess && accessType == AccessType::ReadWrite) ||
        (other.type == MutexType::EntityAccess && other.accessType == AccessType::ReadWrite)) {
      if (type == MutexType::ComponentAccess || other.type == MutexType::ComponentAccess) {
        return true;
      }
    }

    if (type != other.type) {
      return false;
    }

    if (type == MutexType::EntityAccess) {
      return accessType == AccessType::ReadWrite || other.accessType == AccessType::ReadWrite;
    }

    if (type == MutexType::ComponentAccess) {
      return (hashCode == other.hashCode) && (accessType == AccessType::ReadWrite || other.accessType == AccessType::ReadWrite);
    }

    if (type == MutexType::UseClass) {
      return (hashCode == other.hashCode) && (accessType == AccessType::ReadWrite || other.accessType == AccessType::ReadWrite);
    }

    return false;
  }

  FE_FINLINE constexpr static PassMutex ReadEntity();
  FE_FINLINE constexpr static PassMutex WriteEntity();

  template <typename T>
  static constexpr PassMutex ReadComponent();

  template <typename T>
  static constexpr PassMutex WriteComponent();

  template <typename T>
  static constexpr PassMutex UseClassConst();

  template <typename T>
  static constexpr PassMutex UseClassNoConst();
};

}  // namespace fe::engine::ecs

// Impl
namespace fe::engine::ecs {
FE_FINLINE PassId PassId::Create() {
  return PassId(sIdCounter++);
}

FE_FINLINE constexpr PassMutex PassMutex::ReadEntity() {
  PassMutex mutex(MutexType::EntityAccess);
  mutex.accessType = AccessType::Read;
  return mutex;
}
FE_FINLINE constexpr PassMutex PassMutex::WriteEntity() {
  PassMutex mutex(MutexType::EntityAccess);
  mutex.accessType = AccessType::ReadWrite;
  return mutex;
}

template <typename T>
constexpr PassMutex PassMutex::ReadComponent() {
  PassMutex mutex(MutexType::ComponentAccess);
  mutex.accessType = AccessType::Read;
  mutex.hashCode = typeid(T).hash_code();
  return mutex;
}

template <typename T>
constexpr PassMutex PassMutex::WriteComponent() {
  PassMutex mutex(MutexType::ComponentAccess);
  mutex.accessType = AccessType::ReadWrite;
  mutex.hashCode = typeid(T).hash_code();
  return mutex;
}

template <typename T>
constexpr PassMutex PassMutex::UseClassConst() {
  PassMutex mutex(MutexType::UseClass);
  mutex.accessType = AccessType::Read;
  mutex.hashCode = typeid(T).hash_code();
  return mutex;
}

template <typename T>
constexpr PassMutex PassMutex::UseClassNoConst() {
  PassMutex mutex(MutexType::UseClass);
  mutex.accessType = AccessType::ReadWrite;
  mutex.hashCode = typeid(T).hash_code();
  return mutex;
}

}  // namespace fe::engine::ecs

template <>
struct FE_STL_NAMESPACE::hash<fe::engine::ecs::PassId> {
  size_t operator()(const fe::engine::ecs::PassId& id) const { return static_cast<size_t>(id.value); }
};