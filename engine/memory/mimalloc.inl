#include <mimalloc.h>

namespace fe::engine {

template <>
FE_FINLINE void* Allocator<AllocType::MiMalloc>::malloc(size_t size) noexcept {
  return mi_malloc(size);
}

template <>
FE_FINLINE void* Allocator<AllocType::MiMalloc>::zalloc(size_t size) noexcept {
  return mi_zalloc(size);
}

template <>
FE_FINLINE void* Allocator<AllocType::MiMalloc>::calloc(size_t count, size_t size) noexcept {
  return mi_calloc(count, size);
}

template <>
FE_FINLINE void* Allocator<AllocType::MiMalloc>::realloc(void* p, size_t newsize) noexcept {
  return mi_realloc(p, newsize);
}

template <>
FE_FINLINE void* Allocator<AllocType::MiMalloc>::reallocN(void* p, size_t count, size_t newsize) noexcept {
  return mi_reallocn(p, count, newsize);
}

template <>
FE_FINLINE bool Allocator<AllocType::MiMalloc>::expand(void* p, size_t size) noexcept {
  return mi_expand(p, size) != nullptr;
}

template <>
FE_FINLINE void* Allocator<AllocType::MiMalloc>::mallocAligned(size_t size, size_t alignment) noexcept {
  return mi_malloc_aligned(size, alignment);
}

template <>
FE_FINLINE void* Allocator<AllocType::MiMalloc>::zallocAligned(size_t size, size_t alignment) noexcept {
  return mi_zalloc_aligned(size, alignment);
}

template <>
FE_FINLINE void* Allocator<AllocType::MiMalloc>::callocAligned(size_t count, size_t size, size_t alignment) noexcept {
  return mi_calloc_aligned(count, size, alignment);
}

template <>
FE_FINLINE void* Allocator<AllocType::MiMalloc>::reallocAligned(void* p, size_t newsize, size_t alignment) noexcept {
  return mi_realloc_aligned(p, newsize, alignment);
}

template <>
FE_FINLINE void* Allocator<AllocType::MiMalloc>::mallocAlignedAt(size_t size, size_t alignment, size_t offset) noexcept {
  return mi_malloc_aligned_at(size, alignment, offset);
}

template <>
FE_FINLINE void* Allocator<AllocType::MiMalloc>::zallocAlignedAt(size_t size, size_t alignment, size_t offset) noexcept {
  return mi_zalloc_aligned_at(size, alignment, offset);
}

template <>
FE_FINLINE void* Allocator<AllocType::MiMalloc>::callocAlignedAt(size_t count, size_t size, size_t alignment, size_t offset) noexcept {
  return mi_calloc_aligned_at(count, size, alignment, offset);
}

template <>
FE_FINLINE void* Allocator<AllocType::MiMalloc>::reallocAlignedAt(void* p, size_t newSize, size_t alignment, size_t offset) noexcept {
  return mi_realloc_aligned_at(p, newSize, alignment, offset);
}

template <>
FE_FINLINE void Allocator<AllocType::MiMalloc>::free(void* p) noexcept {
  mi_free(p);
}

}  // namespace fe::engine