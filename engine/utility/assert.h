#pragma once

#include "print.h"

#ifdef ERROR
#undef ERROR
#endif
#ifdef ASSERT
#undef ASSERT
#endif
#ifdef HALT
#undef HALT
#endif

#define FE_HALT(...) ERROR(__VA_ARGS__) __debugbreak();

#ifdef RELEASE

#define FE_ASSERT(isTrue, ...) (void)(isTrue)
#define FE_WARN_ONCE_IF(isTrue, ...) (void)(isTrue)
#define FE_WARN_ONCE_IF_NOT(isTrue, ...) (void)(isTrue)
#define FE_ERROR(msg, ...)
#define FE_DEBUGPRINT(msg, ...) \
  do {                          \
  } while (0)
#define FE_ASSERT_SUCCEEDED(hr, ...) (void)(hr)

#else  // !RELEASE

#define FE_STRINGIFY(x) #x
#define FE_STRINGIFY_BUILTIN(x) FE_STRINGIFY(x)
#define FE_ASSERT(isFalse, ...)                                                                                                      \
  do {                                                                                                                               \
    if (!(bool)(isFalse)) {                                                                                                          \
      fe::engine::utility::print("\nAssertion failed in " FE_STRINGIFY_BUILTIN(__FILE__) " @ " FE_STRINGIFY_BUILTIN(__LINE__) "\n"); \
      fe::engine::utility::printSubMessage("\'" #isFalse "\' is false");                                                             \
      fe::engine::utility::printSubMessage(__VA_ARGS__);                                                                             \
      fe::engine::utility::print("\n");                                                                                              \
      __debugbreak();                                                                                                                \
    }                                                                                                                                \
  } while (0)

#define FE_ASSERT_SUCCEEDED(hr, ...)                                                                                               \
  do {                                                                                                                             \
    if (FAILED(hr)) {                                                                                                              \
      fe::engine::utility::print("\nHRESULT failed in " FE_STRINGIFY_BUILTIN(__FILE__) " @ " FE_STRINGIFY_BUILTIN(__LINE__) "\n"); \
      fe::engine::utility::printSubMessage("hr = 0x%08X", hr);                                                                     \
      fe::engine::utility::printSubMessage(__VA_ARGS__);                                                                           \
      fe::engine::utility::print("\n");                                                                                            \
      __debugbreak();                                                                                                              \
    }                                                                                                                              \
  } while (0)

#define FE_WARN_ONCE_IF(isTrue, ...)                                                                                               \
  do {                                                                                                                             \
    static bool s_TriggeredWarning = false;                                                                                        \
    if ((bool)(isTrue) && !s_TriggeredWarning) {                                                                                   \
      s_TriggeredWarning = true;                                                                                                   \
      fe::engine::utility::print("\nWarning issued in " FE_STRINGIFY_BUILTIN(__FILE__) " @ " FE_STRINGIFY_BUILTIN(__LINE__) "\n"); \
      fe::engine::utility::printSubMessage("\'" #isTrue "\' is true");                                                             \
      fe::engine::utility::printSubMessage(__VA_ARGS__);                                                                           \
      fe::engine::utility::print("\n");                                                                                            \
    }                                                                                                                              \
  } while (0)

#define FE_WARN_ONCE_IF_NOT(isTrue, ...) FE_WARN_ONCE_IF(!(isTrue), __VA_ARGS__)

#define FE_ERROR(...)                                                                                                            \
  do {                                                                                                                           \
    fe::engine::utility::print("\nError reported in " FE_STRINGIFY_BUILTIN(__FILE__) " @ " FE_STRINGIFY_BUILTIN(__LINE__) "\n"); \
    fe::engine::utility::printSubMessage(__VA_ARGS__);                                                                           \
    fe::engine::utility::print("\n");                                                                                            \
  } while (0)

#define FE_DEBUGPRINT(msg, ...) fe::engine::utility::printf(msg "\n", ##__VA_ARGS__);

#endif

#define FE_BreakIfFailed(hr) \
  if (FAILED(hr))            \
  __debugbreak()