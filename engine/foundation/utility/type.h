#pragma once

#include <typeindex>

namespace fe::engine {
using TypeId = std::type_index;

template <typename T>
static TypeId get_type_id() {
  static const std::type_index id = typeid(T);
  return id;
}
}  // namespace fe::engine