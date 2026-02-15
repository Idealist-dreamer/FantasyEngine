#include <mimalloc.h>

namespace fe::engine {

FE_FINLINE void* Allocator::malloc(size_t size) noexcept {
  return mi_malloc(size);
}

FE_FINLINE void* Allocator::zalloc(size_t size) noexcept {
  return mi_zalloc(size);
}

FE_FINLINE void* Allocator::calloc(size_t count, size_t size) noexcept {
  return mi_calloc(count, size);
}

FE_FINLINE void* Allocator::realloc(void* p, size_t newsize) noexcept {
  return mi_realloc(p, newsize);
}

FE_FINLINE void* Allocator::reallocN(void* p, size_t count, size_t newsize) noexcept {
  return mi_reallocn(p, count, newsize);
}

FE_FINLINE bool Allocator::expand(void* p, size_t size) noexcept {
  return mi_expand(p, size) != nullptr;
}

FE_FINLINE void* Allocator::mallocAligned(size_t size, size_t alignment) noexcept {
  return mi_malloc_aligned(size, alignment);
}

FE_FINLINE void* Allocator::zallocAligned(size_t size, size_t alignment) noexcept {
  return mi_zalloc_aligned(size, alignment);
}

FE_FINLINE void* Allocator::callocAligned(size_t count, size_t size, size_t alignment) noexcept {
  return mi_calloc_aligned(count, size, alignment);
}

FE_FINLINE void* Allocator::reallocAligned(void* p, size_t newsize, size_t alignment) noexcept {
  return mi_realloc_aligned(p, newsize, alignment);
}

FE_FINLINE void* Allocator::mallocAlignedAt(size_t size, size_t alignment, size_t offset) noexcept {
  return mi_malloc_aligned_at(size, alignment, offset);
}

FE_FINLINE void* Allocator::zallocAlignedAt(size_t size, size_t alignment, size_t offset) noexcept {
  return mi_zalloc_aligned_at(size, alignment, offset);
}

FE_FINLINE void* Allocator::callocAlignedAt(size_t count, size_t size, size_t alignment, size_t offset) noexcept {
  return mi_calloc_aligned_at(count, size, alignment, offset);
}

FE_FINLINE void* Allocator::reallocAlignedAt(void* p, size_t newSize, size_t alignment, size_t offset) noexcept {
  return mi_realloc_aligned_at(p, newSize, alignment, offset);
}

FE_FINLINE void Allocator::free(void* p) noexcept {
  mi_free(p);
}

}  // namespace fe::engine