#pragma once

#include <typeindex>

#include "engine/base/macros.h"
#include "engine/base/memory/allocator.h"

namespace fe::engine::ecs {
struct ResourceId {
  static constexpr uint32_t Invalid = FE_UINT32_MAX;

  ResourceId(uint32_t _value = Invalid, uint32_t _version = 0) : value(_value), version(_version) {}

  bool null() const { return value == Invalid; }
  auto operator<=>(const ResourceId& other) const = default;

  uint32_t value;
  uint32_t version;
};

struct Resource {
  Resource() = default;
  ~Resource() { destroy(); }

  Resource(const Resource&) = delete;
  Resource& operator=(const Resource&) = delete;

  Resource(Resource&& other) noexcept : m_ptr(other.m_ptr), m_deleter(other.m_deleter) {
    other.m_ptr = nullptr;
    other.m_deleter = nullptr;

#if defined(FE_DEBUG)
    m_TypeInfo = other.m_TypeInfo;
    other.m_TypeInfo = typeid(void);
#endif
  }

  Resource& operator=(Resource&& other) noexcept {
    if (this != &other) {
      destroy();
      m_ptr = other.m_ptr;
      m_deleter = other.m_deleter;
      other.m_ptr = nullptr;
      other.m_deleter = nullptr;

#if defined(FE_DEBUG)
      m_TypeInfo = other.m_TypeInfo;
      other.m_TypeInfo = typeid(void);
#endif
    }
    return *this;
  }

  bool valid() const { return m_ptr != nullptr; }

  template <typename T>
  T* get() {
    FE_ASSERT(valid() && m_TypeInfo == typeid(T), "Type mismatch!");
    return static_cast<T*>(m_ptr);
  }

  template <typename T>
  const T* get() const {
    FE_ASSERT(valid() && m_TypeInfo == typeid(T), "Type mismatch!");
    return static_cast<const T*>(m_ptr);
  }

  FE_FINLINE void* getPtr() { return m_ptr; }
  FE_FINLINE const void* getPtr() const { return m_ptr; }

  FE_FINLINE void destroy() {
    if (m_ptr && m_deleter) {
      m_deleter(m_ptr);
    }
    m_ptr = nullptr;
    m_deleter = nullptr;
#if defined(FE_DEBUG)
    m_TypeInfo = typeid(void);
#endif
  }

  template <typename T, typename... Args>
  static Resource create(Args&&... args) {
    Resource res;
    res.m_ptr = memory::Allocator::create<T>(std::forward<Args>(args)...);
    res.m_deleter = [](void* p) {
      if (p) {
        memory::Allocator::destroy<T>(static_cast<T*>(p));
      }
    };
#if defined(FE_DEBUG)
    res.m_TypeInfo = typeid(T);
#endif
    return res;
  }

  template <typename T>
  static Resource create(T* ptr, bool need_free = true) {
    Resource res;
    res.m_ptr = ptr;
    if (need_free) {
      res.m_deleter = [](void* p) {
        if (p) {
          memory::Allocator::destroy<T>(static_cast<T*>(p));
        }
      };
    }
#if defined(FE_DEBUG)
    res.m_TypeInfo = typeid(T);
#endif
    return res;
  }

 private:
  void* m_ptr{nullptr};
  void (*m_deleter)(void*) = nullptr;
#if defined(FE_DEBUG)
  std::type_index m_TypeInfo = typeid(void);
#endif
};
}  // namespace fe::engine::ecs