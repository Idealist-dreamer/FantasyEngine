#pragma once

#include "engine/ecs/system.h"

namespace fe::engine::render {

class RenderGLTF : public System {
 public:
  RenderCore();
  ~RenderCore();

  bool init(Visitor<WorldBase>& visitor) override;

  FE_DECLARE_PRIVATE
};
}  // namespace fe::engine::render