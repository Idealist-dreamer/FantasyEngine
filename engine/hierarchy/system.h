#pragma once

#include "engine/ecs/system.h"

namespace fe::engine::hierarchy {

class HierarchySystem : public System {
  HierarchySystem();
  ~HierarchySystem() override;

  bool init() override;
};

}  // namespace fe::engine::hierarchy