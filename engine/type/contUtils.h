#pragma once

#include "container.h"

#include <typeindex>

#ifdef FE_USE_EASTL
namespace eastl {
// template <typename T>
// struct hash<re::engine::basic_string<T>> {
//   size_t operator()(const re::engine::basic_string<T>& str) const noexcept {
//     return std::hash<std::basic_string_view<T>>{}(std::basic_string_view<T>(str.data(), str.size()));
//   }
// };

template <>
struct hash<std::type_index> {
  size_t operator()(const std::type_index& idx) const noexcept { return idx.hash_code(); }
};
}  // namespace eastl
#endif

namespace fe::engine {}