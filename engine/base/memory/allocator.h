#pragma once

#include <utility>
#include <new>
#include <cstddef>

#include "engine/base/macros.h"

namespace fe::engine::memory {
constexpr size_t kDefaultAlignment = alignof(std::max_align_t);

class Allocator {
 public:
  FE_FINLINE static void* malloc(size_t size) noexcept;
  FE_FINLINE static void* zalloc(size_t size) noexcept;
  FE_FINLINE static void* calloc(size_t count, size_t size) noexcept;

  FE_FINLINE static void* realloc(void* p, size_t newSize) noexcept;
  FE_FINLINE static void* reallocn(void* p, size_t count, size_t newSize) noexcept;
  FE_FINLINE static bool expand(void* p, size_t newSize) noexcept;
  FE_FINLINE static void* malloc_aligned(size_t size, size_t alignment) noexcept;
  FE_FINLINE static void* zalloc_aligned(size_t size, size_t alignment) noexcept;
  FE_FINLINE static void* calloc_aligned(size_t count, size_t size, size_t alignment) noexcept;
  FE_FINLINE static void* realloc_aligned(void* p, size_t newsize, size_t alignment) noexcept;

  FE_FINLINE static void* malloc_aligned_at(size_t size, size_t alignment, size_t offset) noexcept;
  FE_FINLINE static void* zalloc_aligned_at(size_t size, size_t alignment, size_t offset) noexcept;
  FE_FINLINE static void* calloc_aligned_at(size_t count, size_t size, size_t alignment, size_t offset) noexcept;
  FE_FINLINE static void* realloc_aligned_at(void* p, size_t newsize, size_t alignment, size_t offset) noexcept;

  FE_FINLINE static void free(void* p) noexcept;

  template <typename T>
  FE_FINLINE static T* malloc_type() noexcept {
    void* p = nullptr;
    p = malloc(sizeof(T));
    return static_cast<T*>(p);
  }

  template <typename T>
  FE_FINLINE static T* malloc_type_array(size_t count) noexcept {
    size_t alignment = alignof(T);
    void* p = (alignment > kDefaultAlignment) ? malloc_aligned(count * sizeof(T), alignment) : malloc(count * sizeof(T));
    return static_cast<T*>(p);
  }

  template <typename T>
  FE_FINLINE static T* zalloc_type() noexcept {
    void* p = nullptr;
    p = zalloc(sizeof(T));
    return static_cast<T*>(p);
  }

  template <typename T>
  FE_FINLINE static T* zalloc_type_array(size_t count) noexcept {
    size_t alignment = alignof(T);
    void* p = (alignment > kDefaultAlignment) ? zalloc_aligned(count * sizeof(T), alignment) : zalloc(count * sizeof(T));
    return static_cast<T*>(p);
  }

  template <typename T, typename... Args>
  FE_FINLINE static T* create(Args&&... args) {
    size_t alignment = alignof(T);
    void* p = (alignment > kDefaultAlignment) ? malloc_aligned(sizeof(T), alignment) : malloc(sizeof(T));
    if (!p)
      return nullptr;
    return new (p) T(std::forward<Args>(args)...);
  }

  template <typename T, typename... Args>
  FE_FINLINE static T* create_array(size_t count, Args&&... args) {
    T* p = malloc_type_array<T>(count);
    if (!p)
      return nullptr;

    for (size_t i = 0; i < count; ++i) {
      new (&p[i]) T(std::forward<Args>(args)...);
    }
    return p;
  }

  template <typename T>
  FE_FINLINE static void destroy(T* ptr) noexcept {
    if (!ptr) {
      return;
    }

    ptr->~T();
    void* p = static_cast<void*>(ptr);
    free(p);
  }

  template <typename T>
  FE_FINLINE static void destroy_array(T* ptr, size_t count) noexcept {
    if (!ptr)
      return;

    // Destruct in reverse order (standard C++ behavior)
    for (size_t i = count; i > 0; --i) {
      ptr[i - 1].~T();
    }
    free(static_cast<void*>(ptr));
  }
};

}  // namespace fe::engine::memory

#include "allocator.inl"
#include "overNewDele.h"