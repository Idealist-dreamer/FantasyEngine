#include <memory>
#include <mimalloc.h>

namespace fe::engine {

template <AllocType type, typename T>
class StlAllocator;

template <typename T>
class StlAllocator<AllocType::STD, T> : public std::allocator<T> {
 public:
  using std::allocator<T>::allocator;

  StlAllocator(const char* pName) noexcept : std::allocator<T>() { (void)pName; }

  template <typename U>
  StlAllocator(const StlAllocator<AllocType::STD, U>&) noexcept : std::allocator<T>() {}

  template <typename U>
  struct rebind {
    using other = StlAllocator<AllocType::STD, U>;
  };

  FE_FINLINE T* allocate(size_t n, size_t /*alignment*/, size_t /*offset*/, int /*flags*/ = 0) { return std::allocator<T>::allocate(n); }
  FE_FINLINE T* allocate(size_t n, int /*flags*/, const char* /*pName*/ = nullptr) { return std::allocator<T>::allocate(n); }
  FE_FINLINE T* allocate(size_t n) { return std::allocator<T>::allocate(n); }

  FE_FINLINE void deallocate(void* p, size_t n) noexcept {
    if (p) {
      std::allocator<T>::deallocate(static_cast<T*>(p), n);
    }
  }
  FE_FINLINE void deallocate(T* p, size_t n) noexcept { std::allocator<T>::deallocate(p, n); }

  template <typename U>
  FE_FINLINE bool operator==(const StlAllocator<AllocType::STD, U>&) const noexcept {
    return true;
  }
  template <typename U>
  FE_FINLINE bool operator!=(const StlAllocator<AllocType::STD, U>&) const noexcept {
    return false;
  }
};

template <typename T>
class StlAllocator<AllocType::MiMalloc, T> : public mi_stl_allocator<T> {
 public:
  using mi_stl_allocator<T>::mi_stl_allocator;

  StlAllocator(const char* pName) noexcept : mi_stl_allocator<T>() { (void)pName; }

  FE_FINLINE T* allocate(size_t n) { return mi_stl_allocator<T>::allocate(n); }
  FE_FINLINE T* allocate(size_t n, int flags, const char* pName = nullptr) { return mi_stl_allocator<T>::allocate(n); }
  FE_FINLINE T* allocate(size_t n, size_t alignment, size_t offset, int flags = 0) {
    return static_cast<T*>(mi_malloc_aligned(n * sizeof(T), alignment));
  }

  FE_FINLINE void deallocate(void* p, size_t n) noexcept { mi_free(p); }
  FE_FINLINE void deallocate(T* p, size_t n) noexcept { mi_free(p); }

  template <typename U>
  struct rebind {
    using other = StlAllocator<AllocType::MiMalloc, U>;
  };

  template <typename U>
  bool operator==(const StlAllocator<AllocType::MiMalloc, U>&) const noexcept {
    return true;
  }
  template <typename U>
  bool operator!=(const StlAllocator<AllocType::MiMalloc, U>&) const noexcept {
    return false;
  }
};

}  // namespace fe::engine