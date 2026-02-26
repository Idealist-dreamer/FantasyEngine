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
