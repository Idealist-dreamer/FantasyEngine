#pragma once

#include <filesystem>

#include "core/log/log.h"

#if defined(_WIN32)
#define FE_DEBUGBREAK() __debugbreak()
#else
#define FE_DEBUGBREAK() __builtin_trap()
#endif

#define FE_FILENAME (::std::filesystem::path(__FILE__).filename().string())

#define FE_INTERNAL_FATAL_LOG(type, cond_str, ...)                                           \
  do {                                                                                       \
    ::fe::engine::LogManager::Instance().Fatal("=== {0} FAILED ===", #type);                 \
    ::fe::engine::LogManager::Instance().Fatal("Condition: {0}", cond_str);                  \
    ::fe::engine::LogManager::Instance().Fatal("Location:  {0}:{1}", FE_FILENAME, __LINE__); \
    if constexpr (sizeof(#__VA_ARGS__) > 1) {                                                \
      ::fe::engine::LogManager::Instance().Fatal("Message:   " __VA_ARGS__);                 \
    }                                                                                        \
    ::fe::engine::LogManager::Instance().Fatal("==========================");                \
    FE_DEBUGBREAK();                                                                         \
  } while (0)

#define FE_HALT(...) FE_INTERNAL_FATAL_LOG(HALT, "None", ##__VA_ARGS__)

#ifndef FE_RELEASE

#define FE_ASSERT(cond, ...)                                  \
  do {                                                        \
    if (!(cond))                                              \
      FE_INTERNAL_FATAL_LOG(ASSERTION, #cond, ##__VA_ARGS__); \
  } while (0)

#define FE_CORE_ASSERT(cond, ...)                               \
  do {                                                          \
    if (!(cond))                                                \
      FE_INTERNAL_FATAL_LOG(CORE_ASSERT, #cond, ##__VA_ARGS__); \
  } while (0)

#if defined(_WIN32)
#define FE_ASSERT_SUCCEEDED(hr, ...)                      \
  do {                                                    \
    if (FAILED(hr))                                       \
      FE_INTERNAL_FATAL_LOG(HRESULT, #hr, ##__VA_ARGS__); \
  } while (0)
#endif

#define FE_ERROR(...)                                                                      \
  do {                                                                                     \
    ::fe::engine::LogManager::Instance().Error("Error at {0}:{1}", FE_FILENAME, __LINE__); \
    ::fe::engine::LogManager::Instance().Error("  Message: " __VA_ARGS__);                 \
  } while (0)

#define FE_WARN_ONCE_IF(cond, ...)                                                                   \
  do {                                                                                               \
    static bool s_triggered = false;                                                                 \
    if ((cond) && !s_triggered) {                                                                    \
      s_triggered = true;                                                                            \
      ::fe::engine::LogManager::Instance().Warn("Warning (Once) at {0}:{1}", FE_FILENAME, __LINE__); \
      ::fe::engine::LogManager::Instance().Warn("  Message: " __VA_ARGS__);                          \
    }                                                                                                \
  } while (0)

#define FE_WARN_ONCE_IF_NOT(cond, ...) FE_WARN_ONCE_IF(!(cond), ##__VA_ARGS__)

#define FE_DEBUGPRINT(...) ::fe::engine::LogManager::Instance().Debug(__VA_ARGS__)

#else

#define FE_ASSERT(cond, ...) (void)(cond)
#define FE_CORE_ASSERT(cond, ...) (void)(cond)
#define FE_ASSERT_SUCCEEDED(hr, ...) (void)(hr)
#define FE_ERROR(...) (void)0
#define FE_WARN_ONCE_IF(cond, ...) (void)(cond)
#define FE_WARN_ONCE_IF_NOT(cond, ...) (void)(cond)
#define FE_DEBUGPRINT(...) (void)0

#endif

#define FE_ASSERT_NOT_NULL(ptr) FE_ASSERT((ptr) != nullptr, "Pointer '{0}' is null!", #ptr)

#define FE_ASSERT_IN_RANGE(val, min, max) \
  FE_ASSERT((val) >= (min) && (val) <= (max), "Value '{0}' ({1}) out of range [{2}, {3}]", #val, val, min, max)

#define FE_ASSERT_WITH_CODE(cond, code, ...)                           \
  do {                                                                 \
    if (!(cond))                                                       \
      FE_INTERNAL_FATAL_LOG(ASSERT_CODE_##code, #cond, ##__VA_ARGS__); \
  } while (0)

#define FE_BreakIfFailed(hr) \
  do {                       \
    if (FAILED(hr))          \
      FE_DEBUGBREAK();       \
  } while (0)

#define FE_STATIC_ASSERT(cond, msg) static_assert(cond, msg)