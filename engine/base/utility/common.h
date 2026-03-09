#pragma once

namespace fe::engine {

template <typename T, typename... Us>
inline constexpr bool is_in_pack = (std::is_same_v<T, Us> || ...);

template <typename T>
FE_FINLINE bool checkOneOther(T t1, T t2, T x, T& other) {
  if (t1 == x) {
    other = t2;
    return true;
  }
  if (t2 == x) {
    other = t1;
    return true;
  }
  return false;
}
}  // namespace fe::engine