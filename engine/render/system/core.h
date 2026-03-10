#pragma once

#include "engine/ecs/system.h"

namespace fe::engine::render {

class RenderCore : public System {
 public:
  RenderCore();
  ~RenderCore();

  bool init(WorldVisitor& visitor) override;

  FE_DECLARE_PRIVATE
};
}  // namespace fe::engine::render