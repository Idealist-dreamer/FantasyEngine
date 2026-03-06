#pragma once

#include "pass.h"
#include "serialization.h"

namespace fe::engine::ecs {

class System {
 public:
  System(const stl::string& name) : m_name(name) {}
  virtual ~System() = default;

  void set_world(WorldBase& world) { m_world = &world; }

  virtual bool init() = 0;

  virtual void serialize_save(JsonOutputArchive&) {}
  virtual void serialize_load(JsonInputArchive&) {}

  virtual void serialize_save(BinaryOutputArchive&) {}
  virtual void serialize_load(BinaryInputArchive&) {}

 protected:
  stl::string m_name;
  stl::vector<Pass> m_passes;

  WorldBase* m_world = nullptr;

  friend class World;
};

}  // namespace fe::engine::ecs