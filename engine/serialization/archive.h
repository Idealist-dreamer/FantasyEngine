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

namespace fe::engine::serialization {

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

using BinaryOutput = Output<cereal::BinaryOutputArchive>;
using BinaryInput = Input<cereal::BinaryInputArchive>;

using JSONOutput = Output<cereal::JSONOutputArchive>;
using JSONInput = Input<cereal::JSONInputArchive>;

using XMLOutput = Output<cereal::XMLOutputArchive>;
using XMLInput = Input<cereal::XMLInputArchive>;

}  // namespace fe::engine::serialization