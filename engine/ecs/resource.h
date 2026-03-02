#pragma once

#include <typeindex>

#include "engine/base/macros.h"
#include "engine/base/memory/allocator.h"

namespace fe::engine::ecs {
struct ResourceStorage {
  ResourceStorage() = default;
  ~ResourceStorage() { destroy(); }

  ResourceStorage(const ResourceStorage&) = delete;
  ResourceStorage& operator=(const ResourceStorage&) = delete;

  ResourceStorage(ResourceStorage&& other) noexcept : m_ptr(other.m_ptr), m_deleter(other.m_deleter) {
    other.m_ptr = nullptr;
    other.m_deleter = nullptr;

#if defined(FE_DEBUG)
    m_type_info = other.m_type_info;
    other.m_type_info = typeid(void);
#endif
  }

  ResourceStorage& operator=(ResourceStorage&& other) noexcept {
    if (this != &other) {
      destroy();
      m_ptr = other.m_ptr;
      m_deleter = other.m_deleter;
      other.m_ptr = nullptr;
      other.m_deleter = nullptr;

#if defined(FE_DEBUG)
      m_type_info = other.m_type_info;
      other.m_type_info = typeid(void);
#endif
    }
    return *this;
  }

  bool valid() const { return m_ptr != nullptr; }

  template <typename T>
  T* get() {
#if defined(FE_DEBUG)
    FE_ASSERT(valid() && m_type_info == typeid(T), "Type mismatch!");
#endif
    return static_cast<T*>(m_ptr);
  }

  template <typename T>
  const T* get() const {
#if defined(FE_DEBUG)
    FE_ASSERT(valid() && m_type_info == typeid(T), "Type mismatch!");
#endif
    return static_cast<const T*>(m_ptr);
  }

  FE_FINLINE void* get_ptr() { return m_ptr; }
  FE_FINLINE const void* get_ptr() const { return m_ptr; }

  FE_FINLINE void destroy() {
    if (m_ptr && m_deleter) {
      m_deleter(m_ptr);
    }
    m_ptr = nullptr;
    m_deleter = nullptr;
#if defined(FE_DEBUG)
    m_type_info = typeid(void);
#endif
  }

  template <typename T, typename... Args>
  static ResourceStorage create(Args&&... args) {
    ResourceStorage res;
    res.m_ptr = memory::Allocator::create<T>(std::forward<Args>(args)...);
    res.m_deleter = [](void* p) {
      if (p) {
        memory::Allocator::destroy<T>(static_cast<T*>(p));
      }
    };
#if defined(FE_DEBUG)
    res.m_type_info = typeid(T);
#endif
    return res;
  }

  template <typename T>
  static ResourceStorage create(T* ptr, bool need_free = true) {
    ResourceStorage res;
    res.m_ptr = ptr;
    if (need_free) {
      res.m_deleter = [](void* p) {
        if (p) {
          memory::Allocator::destroy<T>(static_cast<T*>(p));
        }
      };
    }
#if defined(FE_DEBUG)
    res.m_type_info = typeid(T);
#endif
    return res;
  }

 private:
  void* m_ptr{nullptr};
  void (*m_deleter)(void*) = nullptr;
#if defined(FE_DEBUG)
  std::type_index m_type_info = typeid(void);
#endif
};

template <typename T>
struct Resource {
  Resource(ResourceStorage& res) : m_resource(res) {}

  bool valid() const { return m_resource.valid(); }

  T& get() { return *m_resource.get<T>(); }
  const T& get() const { return *m_resource.get<T>(); }

  void destroy() { m_resource.destroy(); }

  template <typename T, typename... Args>
  void create(Args&&... args) {
    m_resource = ResourceStorage::create<T>(std::forward<Args>(args)...);
  }

  template <typename T>
  void create(T* ptr, bool need_free = true) {
    m_resource = ResourceStorage::create<T>(ptr, need_free);
  }

 private:
  ResourceStorage& m_resource;
};

}  // namespace fe::engine::ecs