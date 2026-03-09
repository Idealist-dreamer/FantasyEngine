#pragma once

#include "engine/base/container/stl.h"
#include "engine/base/utility/class.h"

class GraphicsContext;

namespace fe::engine::render {

class RenderGltfCore {
  FE_DECLARE_PRIVATE

 public:
  RenderGltfCore();
  ~RenderGltfCore();

  void load_gltf(const stl::string& file);
  void render(GraphicsContext* gfxContext, uint32_t model_id);
};

}  // namespace fe::engine::render