#include "core.h"

#include "engine/render/shared/graphicsCore.h"
#include "engine/render/shared/stage.h"

namespace fe::engine::render {
using namespace ecs;

struct RenderCore::Impl {};

RenderCore::RenderCore() : ecs::System("RenderCore"), m_pImpl(stl::make_unique<Impl>()) {}
RenderCore::~RenderCore() {}

bool RenderCore::init() {
  // auto Pass = Pass::create<InitCore>([](ContextWriter<GraphicsCore> conWriter) {
  //   conWriter.create();

  //   auto& gc = conWriter.get();
  // });

  return true;
}

}  // namespace fe::engine::render