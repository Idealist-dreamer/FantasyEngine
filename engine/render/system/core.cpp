#include "core.h"

#include "engine/render/resource/graphicsCore.h"

namespace fe::engine::render {
struct RenderCore::Impl {};

RenderCore::RenderCore() : ecs::System("RenderCore"), m_pImpl(stl::make_unique<Impl>()) {}
RenderCore::~RenderCore() {}

void createRenderCore(GraphicsCore* gc) {}

bool RenderCore::init() {
  // ecs::Pass pass("createRenderCore", false);
  return true;
}

}  // namespace fe::engine::render