#pragma once

#include "system.h"
#include "stage.h"

namespace fe::engine::ecs {
class World : public WorldBase {
 public:
  World();
  ~World();

  void add_system(stl::shared_ptr<System> sys);

  void compile();
  void setup();
  void run();

  void dump_graph(const stl::string& path);

 private:
  FE_DECLARE_PRIVATE

  void PreStartup();
  void Startup();
  void PostStartup();

  void PreUpdate();
  void Update();
  void PostUpdate();

  void Cleanup();
};
}  // namespace fe::engine::ecs