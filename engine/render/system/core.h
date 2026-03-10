#pragma once

#include "framework/system.h"

namespace fe::engine::render {

class RenderCore : public System {
 public:
  RenderCore();
  ~RenderCore();

  bool init(SceneBase& scene) override;

  FE_DECLARE_PRIVATE
};
}  // namespace fe::engine::render