#pragma once

/**
 * Compilers.
 */
#define FE_COMPILER_MSVC 1
#define FE_COMPILER_CLANG 2
#define FE_COMPILER_GCC 3

#ifndef FE_COMPILER
#if defined(_MSC_VER)
#define FE_COMPILER FE_COMPILER_MSVC
#elif defined(__clang__)
#define FE_COMPILER FE_COMPILER_CLANG
#elif defined(__GNUC__)
#define FE_COMPILER FE_COMPILER_GCC
#else
#error "Unsupported compiler"
#endif
#endif  // FE_COMPILER

#define FE_MSVC (FE_COMPILER == FE_COMPILER_MSVC)
#define FE_CLANG (FE_COMPILER == FE_COMPILER_CLANG)
#define FE_GCC (FE_COMPILER == FE_COMPILER_GCC)

/**
 * Platforms.
 */
#define FE_PLATFORM_WINDOWS 1
#define FE_PLATFORM_LINUX 2

#ifndef FE_PLATFORM
#if defined(_WIN64)
#define FE_PLATFORM FE_PLATFORM_WINDOWS
#elif defined(__linux__)
#define FE_PLATFORM FE_PLATFORM_LINUX
#else
#error "Unsupported target platform"
#endif
#endif  // FE_PLATFORM

#define FE_WINDOWS (FE_PLATFORM == FE_PLATFORM_WINDOWS)
#define FE_LINUX (FE_PLATFORM == FE_PLATFORM_LINUX)

/**
 * D3D12 Agility SDK.
 */
#define FE_D3D12_AGILITY_SDK_PATH ".\\D3D12\\"
// To enable the D3D12 Agility SDK, this macro needs to be added to the main source file of the executable.
#define FE_EXPORT_D3D12_AGILITY_SDK                                                               \
  extern "C" {                                                                                    \
  __declspec(dllexport) extern const unsigned int D3D12SDKVersion = FE_D3D12_AGILITY_SDK_VERSION; \
  }                                                                                               \
  extern "C" {                                                                                    \
  __declspec(dllexport) extern const char* D3D12SDKPath = FE_D3D12_AGILITY_SDK_PATH;              \
  }

// /**
//  * Shared library (DLL) export and import.
//  */
// #ifdef FE_EXPORT
// #define FE_DECLSPEC __declspec(dllexport)
// #else
// #define FE_DECLSPEC __declspec(dllimport)
// #endif

#define FE_API FE_DECLSPEC

/**
 * force inline.
 */
#if FE_MSVC
#define FE_FINLINE __forceinline
#elif FE_CLANG | FE_GCC
#define FE_FINLINE __attribute__((always_inline))
#endif

/**
 * Preprocessor stringification.
 */
#define FE_STRINGIZE(a) #a
#define FE_CONCAT_STRINGS_(a, b) a##b
#define FE_CONCAT_STRINGS(a, b) FE_CONCAT_STRINGS_(a, b)

/**
 * Implement logical operators on a class enum for making it usable as a flags enum.
 */
// clang-format off
#define FE_ENUM_CLASS_OPERATORS(e_) \
    inline e_ operator& (e_ a, e_ b) { return static_cast<e_>(static_cast<int>(a)& static_cast<int>(b)); } \
    inline e_ operator| (e_ a, e_ b) { return static_cast<e_>(static_cast<int>(a)| static_cast<int>(b)); } \
    inline e_& operator|= (e_& a, e_ b) { a = a | b; return a; }; \
    inline e_& operator&= (e_& a, e_ b) { a = a & b; return a; }; \
    inline e_  operator~ (e_ a) { return static_cast<e_>(~static_cast<int>(a)); } \
    inline bool is_set(e_ val, e_ flag) { return (val & flag) != static_cast<e_>(0); } \
    inline void flip_bit(e_& val, e_ flag) { val = is_set(val, flag) ? (val & (~flag)) : (val | flag); }
// clang-format on