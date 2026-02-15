#pragma once

#include "engine/macros.h"

namespace fe::engine {
constexpr size_t kDefaultAlignment = 16;
enum struct AllocType : std::uint8_t { STD, MiMalloc };

/*
* Only suport MiMalloc
*/
template <AllocType type>
class Allocator {
 public:
  // No type alloc
  FE_FINLINE static void* malloc(size_t size) noexcept;
  FE_FINLINE static void* zalloc(size_t size) noexcept;
  FE_FINLINE static void* calloc(size_t count, size_t size) noexcept;

  FE_FINLINE static void* realloc(void* p, size_t newSize) noexcept;
  FE_FINLINE static void* reallocN(void* p, size_t count, size_t newSize) noexcept;
  FE_FINLINE static bool expand(void* p, size_t newSize) noexcept;

  FE_FINLINE static void* mallocAligned(size_t size, size_t alignment) noexcept;
  FE_FINLINE static void* zallocAligned(size_t size, size_t alignment) noexcept;
  FE_FINLINE static void* callocAligned(size_t count, size_t size, size_t alignment) noexcept;
  FE_FINLINE static void* reallocAligned(void* p, size_t newsize, size_t alignment) noexcept;

  FE_FINLINE static void* mallocAlignedAt(size_t size, size_t alignment, size_t offset) noexcept;
  FE_FINLINE static void* zallocAlignedAt(size_t size, size_t alignment, size_t offset) noexcept;
  FE_FINLINE static void* callocAlignedAt(size_t count, size_t size, size_t alignment, size_t offset) noexcept;
  FE_FINLINE static void* reallocAlignedAt(void* p, size_t newsize, size_t alignment, size_t offset) noexcept;

  FE_FINLINE static void free(void* p) noexcept;

  // Type alloc
  template <typename T>
  FE_FINLINE static T* mallocType() noexcept {
    void* p = nullptr;
    p = malloc(sizeof(T));
    return static_cast<T*>(p);
  }

  template <typename T>
  FE_FINLINE static T* mallocTypeArray(size_t count) noexcept {
    size_t alignment = alignof(T);
    void* p = (alignment > kDefaultAlignment) ? mallocAligned(count * sizeof(T), alignment) : malloc(count * sizeof(T));
    return static_cast<T*>(p);
  }

  template <typename T>
  FE_FINLINE static T* zallocType() noexcept {
    void* p = nullptr;
    p = zalloc(sizeof(T));
    return static_cast<T*>(p);
  }

  template <typename T>
  FE_FINLINE static T* zallocTypeArray(size_t count) noexcept {
    size_t alignment = alignof(T);
    void* p = (alignment > kDefaultAlignment) ? zallocAligned(count * sizeof(T), alignment) : zalloc(count * sizeof(T));
    return static_cast<T*>(p);
  }

  template <typename T, typename... Args>
  FE_FINLINE static T* create(Args&&... args) {
    size_t alignment = alignof(T);
    void* p = (alignment > kDefaultAlignment) ? mallocAligned(sizeof(T), alignment) : malloc(sizeof(T));
    if (!p)
      return nullptr;
    return new (p) T(std::forward<Args>(args)...);
  }

  template <typename T, typename... Args>
  FE_FINLINE static T* createArray(size_t count, Args&&... args) {
    T* p = mallocTypeArray<T>(count);
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
  FE_FINLINE static void destroyArray(T* ptr, size_t count) noexcept {
    if (!ptr)
      return;

    // Destruct in reverse order (standard C++ behavior)
    for (size_t i = count; i > 0; --i) {
      ptr[i - 1].~T();
    }
    free(static_cast<void*>(ptr));
  }

  template <typename T>
  struct Deleter {
    void operator()(T* p) const noexcept { destroy<T>(p); }
  };

  template <template <typename> typename SharedPtr, typename T>
  using temp_shared_ptr = SharedPtr<T>;

  template <template <typename, typename> typename UniquePtr, typename T>
  using temp_unique_ptr = UniquePtr<T, Deleter<T>>;
};

}  // namespace fe::engine

// Convenient macro
#define FE_CLASS_ALLOCATOR(_Type) using Alloc = fe::engine::Allocator<fe::engine::AllocType::_Type>;
#define FE_NAME_ALLOCATOR(_Name, _Type) using _Name = fe::engine::Allocator<fe::engine::AllocType::_Type>

#include "mimalloc.inl"
#include "stlAllocator.inl"
#include "overNewDele.h"