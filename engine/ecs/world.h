#pragma once

#include "system.h"

namespace fe::engine::ecs {
class World : public WorldBase {
 public:
  World();
  ~World();

  void add_system(stl::shared_ptr<System> sys);

  void compile();
  void run();

  void dump_graph(const stl::string& path);

  FE_DECLARE_PRIVATE
};
}  // namespace fe::engine::ecs