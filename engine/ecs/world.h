#pragma once

#include "system.h"

namespace fe::engine::ecs {
class World : public WorldBase {
 public:
  World();
  ~World();

  void addSystem(stl::shared_ptr<System> sys);

  void compile();
  void run();

  void dumpGraph(const stl::string& path);

  FE_DECLARE_PRIVATE
};
}  // namespace fe::engine::ecs