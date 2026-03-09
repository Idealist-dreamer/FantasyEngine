#pragma once

#include "system.h"

namespace fe::engine {
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
  void Init();
  void PreStartup();
  void Startup();
  void PostStartup();

  void First();
  void PreUpdate();
  void Update();
  void PostUpdate();

  void Last();
  void Cleanup();

  FE_DECLARE_PRIVATE
};
}  // namespace fe::engine