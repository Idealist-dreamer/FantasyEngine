#pragma once

#include "foundation/macros.h"

#define FE_DECLARE_PRIVATE                       \
  struct Impl;                                   \
  stl::unique_ptr<Impl> m_pImpl;                 \
                                                 \
 protected:                                      \
  FE_FINLINE Impl* d() { return m_pImpl.get(); } \
  FE_FINLINE const Impl* d() const { return m_pImpl.get(); }

#define FE_DECLARE_PRIVATE_INIT m_pImpl = stl::make_unique<Impl>();

namespace fe::engine {

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