#pragma once

#include "pass.h"
#include "serialization.h"

namespace fe::engine {

class System {
 public:
  System(const stl::string& name) : m_name(name) {}
  virtual ~System() = default;

  /// 初始化系统，通过 WorldVisitor 接口访问世界资源
  virtual bool init(WorldVisitor& visitor) = 0;

  virtual void serialize_save(JsonOutputArchive&) {}
  virtual void serialize_load(JsonInputArchive&) {}

  virtual void serialize_save(BinaryOutputArchive&) {}
  virtual void serialize_load(BinaryInputArchive&) {}

 protected:
  stl::string m_name;
  stl::vector<Pass> m_passes;

  friend class World;
};

}  // namespace fe::engine