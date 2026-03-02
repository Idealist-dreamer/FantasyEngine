#pragma once

#include "engine/ecs/system.h"

namespace fe::engine::render {

class RenderCore : public ecs::System {
 public:
  RenderCore();
  ~RenderCore();

  bool init() override;

  FE_DECLARE_PRIVATE
};
}  // namespace fe::engine::render