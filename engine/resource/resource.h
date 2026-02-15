#pragma once

#include "engine/macros.h"

#include <typeindex>

namespace fe::engine::resource {
struct Resource {
  Resource() = default;
  ~Resource() { Destroy(); }

  Resource(const Resource&) = delete;
  Resource& operator=(const Resource&) = delete;

  Resource(Resource&& other) noexcept : m_Ptr(other.m_Ptr), m_Deleter(other.m_Deleter) {
    other.m_Ptr = nullptr;
    other.m_Deleter = nullptr;

#if defined(FE_DEBUG)
    m_TypeInfo = other.m_TypeInfo;
    other.m_TypeInfo = typeid(void);
#endif
  }

  Resource& operator=(Resource&& other) noexcept {
    if (this != &other) {
      Destroy();
      m_Ptr = other.m_Ptr;
      m_Deleter = other.m_Deleter;
      other.m_Ptr = nullptr;
      other.m_Deleter = nullptr;

#if defined(FE_DEBUG)
      m_TypeInfo = other.m_TypeInfo;
      other.m_TypeInfo = typeid(void);
#endif
    }
    return *this;
  }

  template <typename T>
  void SetPtr(T* ptr, bool needFree = true) {
    Destroy();

    if (ptr != nullptr) {
#if defined(FE_DEBUG)
      m_TypeInfo = typeid(T);
#endif

      if (needFree) {
        m_Deleter = [](void* p) {
          if (p) {
            GAlloc::destroy<T>(static_cast<T*>(p));
          }
        };
      }

      m_Ptr = ptr;
    }
  }

  template <typename T, typename... Args>
  T* Create(Args&&... args) {
    T* ptr = GAlloc::create<T>(std::forward<Args>(args)...);
    SetPtr<T>(ptr, true);
    return ptr;
  }

  bool Valid() const { return m_Ptr != nullptr; }

  template <typename T>
  const T* Get() const {
    FE_ASSERT(Valid() && m_TypeInfo == typeid(T), "Type mismatch!");
    return static_cast<const T*>(m_Ptr);
  }

  template <typename T>
  T* Get() {
    return const_cast<T*>(static_cast<const Resource*>(this)->Get<T>());
  }

  FE_FINLINE const void* GetPtr() const { return m_Ptr; }
  FE_FINLINE void* GetPtr() { return const_cast<void*>(static_cast<const Resource*>(this)->GetPtr()); }

  FE_FINLINE void Destroy() {
    if (m_Ptr && m_Deleter) {
      m_Deleter(m_Ptr);
    }
    m_Ptr = nullptr;
    m_Deleter = nullptr;
#if defined(FE_DEBUG)
    m_TypeInfo = typeid(void);
#endif
  }

 private:
  void* m_Ptr{nullptr};
  void (*m_Deleter)(void*) = nullptr;
#if defined(FE_DEBUG)
  std::type_index m_TypeInfo = typeid(void);
#endif
};
}  // namespace fe::engine::resource