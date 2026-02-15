#pragma once

#include "engine/memory/allocator.h"

#include "stl.h"

namespace fe::engine {
constexpr AllocType PointerAllocType = AllocType::MiMalloc;
using PointerAlloc = Allocator<PointerAllocType>;

template <typename T>
using PointerAllocSTL = StlAllocator<PointerAllocType, T>;

template <typename T>
using shared_ptr = PointerAlloc::temp_shared_ptr<stl::shared_ptr, T>;

template <typename T>
using unique_ptr = PointerAlloc::temp_unique_ptr<stl::unique_ptr, T>;

template <typename T, typename... Args>
FE_FINLINE static shared_ptr<T> make_shared(Args&&... args) {
  return stl::allocate_shared<T, PointerAllocSTL<T>>(PointerAllocSTL<T>(), std::forward<Args>(args)...);
}

template <typename T, typename... Args>
FE_FINLINE static unique_ptr<T> make_unique(Args&&... args) {
  T* ptr = PointerAlloc::create<T>(stl::forward<Args>(args)...);
  return unique_ptr<T>(ptr);
}
}  // namespace fe::engine