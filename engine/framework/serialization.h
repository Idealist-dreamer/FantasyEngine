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

#include <entt/entt.hpp>
#include <iostream>
#include <type_traits>
#include <variant>

#include "native_type.h"

namespace fe::engine {

#define FE_MAKE_NVP(val) ::cereal::make_nvp(#val, val)
#define FE_MAKE_BINARY_DATA(ptr, size) ::cereal::binary_data(ptr, size)

using AnyArchive =
    std::variant<cereal::BinaryOutputArchive*, cereal::JSONOutputArchive*,
                 cereal::BinaryInputArchive*, cereal::JSONInputArchive*>;

class Archive {
 public:
  explicit Archive(AnyArchive ar, Registry* reg = nullptr)
      : ar_(ar), reg_(reg) {
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
      std::visit(
          [&](auto* concreteAr) { (*concreteAr)(std::forward<Args>(args)...); },
          ar_);
      return true;
    } catch (const cereal::Exception& e) {
      return false;
    }
  }

  void entities() {
    if (!reg_) return;
    std::visit(
        [this](auto* concreteAr) {
          using ArType = std::remove_pointer_t<decltype(concreteAr)>;
          if constexpr (std::is_same_v<ArType, cereal::BinaryOutputArchive> ||
                        std::is_same_v<ArType, cereal::JSONOutputArchive>) {
            snapshot_->template get<entt::entity>(*concreteAr);
          } else if constexpr (std::is_same_v<ArType,
                                              cereal::BinaryInputArchive> ||
                               std::is_same_v<ArType,
                                              cereal::JSONInputArchive>) {
            loader_->template get<entt::entity>(*concreteAr);
          }
        },
        ar_);
  }

  template <typename... Components>
  void components() {
    if (!reg_) return;
    std::visit(
        [this](auto* concreteAr) {
          using ArType = std::remove_pointer_t<decltype(concreteAr)>;

          if constexpr (std::is_same_v<ArType, cereal::BinaryOutputArchive> ||
                        std::is_same_v<ArType, cereal::JSONOutputArchive>) {
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