#pragma once

#include "pass.h"

namespace fe::engine::ecs {

class System {
 public:
  System(const stl::string& name = "") : m_name(name) {}
  virtual ~System() = default;

  virtual void init(WorldBase& world) {};

  stl::string m_name;
  stl::vector<Pass> m_passes;
};

}  // namespace fe::engine::ecs