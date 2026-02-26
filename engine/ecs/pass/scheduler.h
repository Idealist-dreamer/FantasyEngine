#pragma once

#include "pass.h"

namespace fe::engine::ecs {
class Scheduler {
 public:
  Scheduler();
  ~Scheduler();

  void compile(const stl::vector<stl::shared_ptr<Pass>>& passes);
  void execute();
  void dumpGraph(const stl::string& path);

  FE_DECLARE_PRIVATE
};
}  // namespace fe::engine::ecs