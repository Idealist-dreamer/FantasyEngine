#pragma once

#include "worldBase.h"
#include "system.h"

#include "pass/scheduler.h"

namespace fe::engine::ecs {
class World : public WorldBase {
 public:
  World() = default;

  void addSystem(const stl::string& name, stl::shared_ptr<System> sys) {
    sys->attach(this);
    m_systems.insert({name, sys});
  }

  void run() {
    if (m_fisrtRun) {
      stl::vector<stl::shared_ptr<Pass>> allPasses;
      for (auto& [name, sys] : m_systems) {
        auto& passes = sys->getPasses();
        allPasses.insert(allPasses.end(), passes.begin(), passes.end());
      }

      m_scheduler.compile(allPasses);

      m_fisrtRun = false;
    }

    m_scheduler.execute();
  }

 private:
  stl::unordered_map<stl::string, stl::shared_ptr<System>> m_systems;

  bool m_fisrtRun = true;
  Scheduler m_scheduler;
};
}  // namespace fe::engine::ecs