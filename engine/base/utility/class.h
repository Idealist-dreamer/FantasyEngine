#pragma once

#define FE_DECLARE_PRIVATE           \
 private:                            \
  struct Impl;                       \
  stl::unique_ptr<Impl> m_pImpl;     \
                                     \
 protected:                          \
  FE_FINLINE Impl* d() {             \
    return m_pImpl.get();            \
  }                                  \
  FE_FINLINE const Impl* d() const { \
    return m_pImpl.get();            \
  }

#define FE_DECLARE_PRIVATE_INIT m_pImpl = stl::make_unique<Impl>();

namespace fe::engine {
template <typename T, typename... Us>
inline constexpr bool is_in_pack = (std::is_same_v<T, Us> || ...);
}