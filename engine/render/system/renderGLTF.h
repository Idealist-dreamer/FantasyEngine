#pragma once

#include "framework/system.h"

namespace fe::engine::render {

class RenderGLTF : public System {
 public:
  RenderGLTF();
  ~RenderGLTF();

  bool init(SceneBase& scene) override;

  FE_DECLARE_PRIVATE
};
}  // namespace fe::engine::render
