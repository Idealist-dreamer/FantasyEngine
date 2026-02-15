#pragma once

#include "engine/memory/allocator.h"

#include "stl.h"

namespace fe::engine {
constexpr AllocType ContinerAllocType = AllocType::MiMalloc;
using ContinerAlloc = Allocator<ContinerAllocType>;

template <typename T>
using ContinerAllocSTL = StlAllocator<ContinerAllocType, T>;

template <typename T>
using vector = stl::vector<T, ContinerAllocSTL<T>>;

template <typename T>
using deque = stl::deque<T, ContinerAllocSTL<T>>;

template <typename T>
using list = stl::list<T, ContinerAllocSTL<T>>;

template <typename T, size_t N>
using array = stl::array<T, N>;

template <typename T>
using set = stl::set<T, stl::less<T>, ContinerAllocSTL<T>>;

template <typename Key, typename Value>
using map = stl::map<Key, Value, stl::less<Key>, ContinerAllocSTL<stl::pair<const Key, Value>>>;

template <typename T>
using multiset = stl::multiset<T, stl::less<T>, ContinerAllocSTL<T>>;

template <typename T>
using hash_set = stl::unordered_set<T, stl::hash<T>, stl::equal_to<T>, ContinerAllocSTL<T>>;

template <typename Key, typename Value>
using hash_map = stl::unordered_map<Key, Value, stl::hash<Key>, stl::equal_to<Key>, ContinerAllocSTL<stl::pair<const Key, Value>>>;

template <typename Key, typename Value>
using unordered_map = hash_map<Key, Value>;

template <typename T>
using queue = stl::queue<T, deque<T>>;

template <typename T>
using stack = stl::stack<T, deque<T>>;

template <typename T, typename Container = vector<T>, typename Compare = stl::less<typename Container::value_type>>
using priority_queue = stl::priority_queue<T, Container, Compare>;

template <typename T>
using basic_string = stl::basic_string<T, ContinerAllocSTL<T>>;

using string = basic_string<char>;
using wstring = basic_string<wchar_t>;

using u8string = basic_string<char8_t>;
using u16string = basic_string<char16_t>;
}  // namespace fe::engine