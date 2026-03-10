#pragma once

#include "core/macros.h"

#ifdef FE_USE_EASTL
#define FE_STL_NAMESPACE eastl

FE_FINLINE void* operator new[](size_t size, const char* name, int flags, unsigned debugFlags, const char* file, int line);
FE_FINLINE void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* name, int flags, unsigned debugFlags,
                                const char* file, int line);
FE_FINLINE void* operator new(size_t size, const char* name, int flags, unsigned debugFlags, const char* file, int line);
FE_FINLINE void* operator new(size_t size, size_t alignment, size_t alignmentOffset, const char* name, int flags, unsigned debugFlags,
                              const char* file, int line);

#include <EASTL/vector.h>
#include <EASTL/queue.h>
#include <EASTL/string.h>
#include <EASTL/deque.h>
#include <EASTL/list.h>
#include <EASTL/stack.h>
#include <EASTL/array.h>
#include <EASTL/string.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/fixed_string.h>

#include <EASTL/map.h>
#include <EASTL/set.h>
#include <EASTL/unordered_map.h>
#include <EASTL/unordered_set.h>
#include <EASTL/priority_queue.h>

#include <EASTL/shared_ptr.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/weak_ptr.h>
#include <EASTL/memory.h>
#include <EASTL/initializer_list.h>

#include <EASTL/algorithm.h>
#include <EASTL/functional.h>
#include <EASTL/type_traits.h>
#include <EASTL/utility.h>
#include <EASTL/numeric.h>
#include <EASTL/iterator.h>

#include <EASTL/string_view.h>
#include <EASTL/span.h>
#include <EASTL/optional.h>
#include <EASTL/variant.h>
#include <EASTL/any.h>
#include <EASTL/bit.h>
#include <EASTL/tuple.h>

#include <EASTL/atomic.h>

#else
#define FE_STL_NAMESPACE std

#include <vector>
#include <queue>
#include <string>
#include <deque>
#include <list>
#include <stack>
#include <array>

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <memory>
#include <scoped_allocator>

#include <algorithm>
#include <functional>
#include <type_traits>
#include <utility>
#include <numeric>
#include <iterator>
#include <limits>

#include <string_view>
#include <span>
#include <optional>
#include <variant>
#include <any>
#include <bit>
#include <tuple>
#include <format>

#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#endif

namespace fe::engine::stl {
using namespace FE_STL_NAMESPACE;
}

#ifdef FE_USE_EASTL
FE_FINLINE void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line) {
  return ::operator new(size);
}
FE_FINLINE void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags,
                                const char* file, int line) {
  return ::operator new[](size, std::align_val_t(alignment));
}
FE_FINLINE void* operator new(size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line) {
  return ::operator new(size);
}
FE_FINLINE void* operator new(size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags,
                              const char* file, int line) {
  return ::operator new(size, std::align_val_t(alignment));
}
#endif

#include <cstdio>
#include <cwchar>
#include <stdarg.h>

FE_FINLINE int Vsnprintf8(char* pDestination, size_t n, const char* pFormat, va_list arguments) {
  return vsnprintf(pDestination, n, pFormat, arguments);
}

FE_FINLINE int Vsnprintf16(char16_t* pDestination, size_t n, const char16_t* pFormat, va_list arguments) {
#ifdef _MSC_VER
  return _vsnwprintf((wchar_t*)pDestination, n, (const wchar_t*)pFormat, arguments);
#else
  return vswprintf((wchar_t*)pDestination, n, (const wchar_t*)pFormat, arguments);
#endif
}

FE_FINLINE int Vsnprintf32(char32_t* pDestination, size_t n, const char32_t* pFormat, va_list arguments) {
#ifdef _MSC_VER
  return -1;
#else
  return vswprintf((wchar_t*)pDestination, n, (const wchar_t*)pFormat, arguments);
#endif
}

#if defined(EA_CHAR8_UNIQUE) && EA_CHAR8_UNIQUE
FE_FINLINE int Vsnprintf8(char8_t* pDestination, size_t n, const char8_t* pFormat, va_list arguments) {
  return vsnprintf((char*)pDestination, n, (const char*)pFormat, arguments);
}
#endif

#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
FE_FINLINE int VsnprintfW(wchar_t* pDestination, size_t n, const wchar_t* pFormat, va_list arguments) {
#ifdef _MSC_VER
  return _vsnwprintf(pDestination, n, pFormat, arguments);
#else
  return vswprintf(pDestination, n, pFormat, arguments);
#endif
}
#endif

#include <typeindex>

template <>
struct FE_STL_NAMESPACE::hash<std::type_index> {
  size_t operator()(std::type_index val) const { return val.hash_code(); }
};