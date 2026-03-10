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
#include <variant>

namespace fe::engine {

#define FE_MAKE_NVP(val) ::cereal::make_nvp(#val, val)
#define FE_MAKE_BINARY_DATA(ptr, size) ::cereal::binary_data(ptr, size)

using AnyArchive = std::variant<cereal::BinaryOutputArchive*, cereal::JSONOutputArchive*, cereal::BinaryInputArchive*, cereal::JSONInputArchive*>;

class Archive {
 public:
  explicit Archive(AnyArchive ar, Registry* reg = nullptr) : ar_(ar), reg_(reg) {
    if (reg_) {
      if (is_output()) {
        snapshot_ = std::make_unique<entt::snapshot>(*reg_);
      } else {
        loader_ = std::make_unique<entt::snapshot_loader>(*reg_);
      }
    }
  }

  template <typename... Args>
  bool operator()(Args&&... args) {
    try {
      std::visit([&](auto* concreteAr) { (*concreteAr)(std::forward<Args>(args)...); }, ar_);
      return true;
    } catch (const cereal::Exception& e) {
      return false;
    }
  }

  void entities() {
    if (!reg_)
      return;
    std::visit(
        [this](auto* concreteAr) {
          using ArType = std::remove_pointer_t<decltype(concreteAr)>;
          if constexpr (std::is_same_v<ArType, cereal::BinaryOutputArchive> || std::is_same_v<ArType, cereal::JSONOutputArchive>) {
            snapshot_->template get<entt::entity>(*concreteAr);
          } else if constexpr (std::is_same_v<ArType, cereal::BinaryInputArchive> || std::is_same_v<ArType, cereal::JSONInputArchive>) {
            loader_->template get<entt::entity>(*concreteAr);
          }
        },
        ar_);
  }

  template <typename... Components>
  void components() {
    if (!reg_)
      return;
    std::visit(
        [this](auto* concreteAr) {
          using ArType = std::remove_pointer_t<decltype(concreteAr)>;

          if constexpr (std::is_same_v<ArType, cereal::BinaryOutputArchive> || std::is_same_v<ArType, cereal::JSONOutputArchive>) {
            if constexpr (sizeof...(Components) > 0) {
              ((snapshot_->template get<Components>(*concreteAr)), ...);
            }
          } else {
            if constexpr (sizeof...(Components) > 0) {
              ((loader_->template get<Components>(*concreteAr)), ...);
            }
          }
        },
        ar_);
  }

  bool is_input() const { return ar_.index() >= 2; }
  bool is_output() const { return ar_.index() < 2; }
  bool is_json() const { return ar_.index() == 1 || ar_.index() == 3; }
  bool is_binary() const { return !is_json(); }

 private:
  AnyArchive ar_;
  Registry* reg_ = nullptr;

  std::unique_ptr<entt::snapshot> snapshot_;
  std::unique_ptr<entt::snapshot_loader> loader_;
};

}  // namespace fe::engine

#ifdef FE_USE_EASTL
#include <cereal/cereal.hpp>
#include <cereal/details/helpers.hpp>

#include "core/container/stl.h"

namespace cereal {

using size_type = uint64_t;

// eastl::string serialization:
// - Binary: use binary_data for efficiency
// - JSON/XML: convert to std::string for proper string representation
template <class Archive, class T, class Allocator>
inline void save(Archive& ar, eastl::basic_string<T, Allocator> const& str) {
  if constexpr (std::is_same_v<Archive, cereal::BinaryOutputArchive>) {
    ar(make_size_tag(static_cast<size_type>(str.size())));
    ar(binary_data(str.data(), str.size() * sizeof(T)));
  } else {
    // JSON/XML: convert to std::string for proper serialization
    std::basic_string<T> stdStr(str.begin(), str.end());
    ar(stdStr);
  }
}

template <class Archive, class T, class Allocator>
inline void load(Archive& ar, eastl::basic_string<T, Allocator>& str) {
  if constexpr (std::is_same_v<Archive, cereal::BinaryInputArchive>) {
    size_type size;
    ar(make_size_tag(size));
    str.resize(static_cast<size_t>(size));
    ar(binary_data(str.data(), static_cast<size_t>(size) * sizeof(T)));
  } else {
    // JSON/XML: load as std::string then convert
    std::basic_string<T> stdStr;
    ar(stdStr);
    str.assign(stdStr.c_str(), stdStr.length());
  }
}

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

template <class Archive, class T, class D>
inline void serialize(Archive& ar, eastl::unique_ptr<T, D>& ptr) {
  T* rawPtr = ptr.get();
  ar(rawPtr);
  if (Archive::is_loading::value)
    ptr.reset(rawPtr);
}

}  // namespace cereal
#endif