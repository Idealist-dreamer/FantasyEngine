#include "renderGLTF.h"

#include "engine/render/shared/graphicsCore.h"
#include "engine/render/shared/stage.h"

namespace fe::engine::render {

struct RenderGLTF::Impl {};

RenderGLTF::RenderGLTF() : System("RenderGLTF"), m_pImpl(stl::make_unique<Impl>()) {}
RenderGLTF::~RenderGLTF() {}

bool RenderGLTF::init(Visitor<WorldBase>& visitor) {

  return true;
}

}  // namespace fe::engine::render