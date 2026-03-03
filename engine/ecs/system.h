#pragma once

#include "pass.h"

namespace fe::engine::ecs {

class System {
 public:
  System(const stl::string& name) : m_name(name) {}
  virtual ~System() = default;

  void setWorld(WorldBase& world) { m_world = &world; }

  virtual bool init() = 0;

  const stl::string& name() const { return m_name; }
  const stl::vector<Pass>& passes() const { return m_passes; }

 protected:
  stl::string m_name;
  stl::vector<Pass> m_passes;

  WorldBase* m_world = nullptr;

  friend class World;
};

}  // namespace fe::engine::ecs