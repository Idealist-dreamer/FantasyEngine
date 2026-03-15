#pragma once

#include "foundation/memory/allocator.h"
#include "foundation/utility/assert.h"

namespace fe::engine {

struct Any {
  Any() = default;
  ~Any() { destroy(); }

  Any(const Any&) = delete;
  Any& operator=(const Any&) = delete;

  Any(Any&& other) noexcept : m_ptr(other.m_ptr), m_deleter(other.m_deleter) {
    other.m_ptr = nullptr;
    other.m_deleter = nullptr;

#if defined(FE_DEBUG)
    m_type_info = other.m_type_info;
    other.m_type_info = typeid(void);
#endif
  }

  Any& operator=(Any&& other) noexcept {
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
  static Any create(Args&&... args) {
    Any res;
    res.m_ptr = Allocator::create<T>(std::forward<Args>(args)...);
    res.m_deleter = [](void* p) {
      if (p) {
        Allocator::destroy<T>(static_cast<T*>(p));
      }
    };
#if defined(FE_DEBUG)
    res.m_type_info = typeid(T);
#endif
    return res;
  }

  template <typename T>
  static Any create(T* ptr, bool need_free = false) {
    Any res;
    res.m_ptr = ptr;
    if (need_free) {
      res.m_deleter = [](void* p) {
        if (p) {
          Allocator::destroy<T>(static_cast<T*>(p));
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
}  // namespace fe::engine