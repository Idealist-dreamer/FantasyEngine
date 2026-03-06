#pragma once

#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/xml.hpp>

#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/set.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/array.hpp>

#include <iostream>
#include <type_traits>

namespace fe::engine::ecs {

#define CORE_NVP(val) ::cereal::make_nvp(#val, val)
#define CORE_BINARY_DATA(ptr, size) ::cereal::binary_data(ptr, size)

template <typename TArchive>
class Output {
 public:
  explicit Output(std::ostream& os) : ar_(os) {}

  template <typename... Args>
  void operator()(Args&&... args) {
    ar_(std::forward<Args>(args)...);
  }

 private:
  TArchive ar_;
};

template <typename TArchive>
class Input {
 public:
  explicit Input(std::istream& is) : ar_(is) {}

  template <typename... Args>
  bool operator()(Args&&... args) {
    try {
      ar_(std::forward<Args>(args)...);
      return true;
    } catch (const cereal::Exception& e) {
      return false;
    }
  }

 private:
  TArchive ar_;
};

using BinaryOutputArchive = Output<cereal::BinaryOutputArchive>;
using BinaryInputArchive = Input<cereal::BinaryInputArchive>;

using JsonOutputArchive = Output<cereal::JSONOutputArchive>;
using JsonInputArchive = Input<cereal::JSONInputArchive>;

}  // namespace fe::engine::ecs

#ifdef FE_USE_EASTL
#include <cereal/cereal.hpp>
#include <cereal/details/helpers.hpp>

#include "engine/base/container/stl.h"

namespace cereal {

// --- 通用：处理容器大小 ---
using size_type = uint64_t;

// --- Vector 适配 (含 POD 性能优化) ---
template <class Archive, class T, class Allocator>
inline void save(Archive& ar, eastl::vector<T, Allocator> const& vector) {
  ar(make_size_tag(static_cast<size_type>(vector.size())));
  if constexpr (std::is_trivially_copyable_v<T> && traits::is_output_serializable<BinaryData<T>, Archive>::value) {
    ar(binary_data(vector.data(), vector.size() * sizeof(T)));
  } else {
    for (auto const& i : vector)
      ar(i);
  }
}

template <class Archive, class T, class Allocator>
inline void load(Archive& ar, eastl::vector<T, Allocator>& vector) {
  size_type size;
  ar(make_size_tag(size));
  vector.resize(static_cast<size_t>(size));
  if constexpr (std::is_trivially_copyable_v<T> && traits::is_input_serializable<BinaryData<T>, Archive>::value) {
    ar(binary_data(vector.data(), static_cast<size_t>(size) * sizeof(T)));
  } else {
    for (auto& i : vector)
      ar(i);
  }
}

// --- String 适配 (始终使用 BinaryData 优化) ---
template <class Archive, class T, class Allocator>
inline void save(Archive& ar, eastl::basic_string<T, Allocator> const& str) {
  // 使用 cereal 风格的 size 记录
  ar(make_size_tag(static_cast<size_type>(str.size())));
  // 写入二进制数据
  ar(binary_data(str.data(), str.size() * sizeof(T)));
}

template <class Archive, class T, class Allocator>
inline void load(Archive& ar, eastl::basic_string<T, Allocator>& str) {
  size_type size;
  ar(make_size_tag(size));
  str.resize(static_cast<size_t>(size));
  ar(binary_data(str.data(), static_cast<size_t>(size) * sizeof(T)));
}

// --- Map/Set 适配 (通用迭代) ---
template <class Archive, typename... Args>
inline void save(Archive& ar, eastl::map<Args...> const& container) {
  ar(make_size_tag(static_cast<size_type>(container.size())));
  for (auto const& i : container)
    ar(i);
}

template <class Archive, typename... Args>
inline void load(Archive& ar, eastl::map<Args...>& container) {
  size_type size;
  ar(make_size_tag(size));
  container.clear();
  for (size_t i = 0; i < size; ++i) {
    typename eastl::map<Args...>::value_type vt;
    ar(vt);
    container.insert(eastl::move(vt));
  }
}

// --- Smart Pointers (UniquePtr 示例) ---
template <class Archive, class T, class D>
inline void serialize(Archive& ar, eastl::unique_ptr<T, D>& ptr) {
  T* rawPtr = ptr.get();
  ar(rawPtr);
  if (Archive::is_loading::value)
    ptr.reset(rawPtr);
}

}  // namespace cereal
#endif