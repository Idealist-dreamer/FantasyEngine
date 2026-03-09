#include "core.h"

#include "engine/render/shared/graphicsCore.h"
#include "engine/render/shared/stage.h"

namespace fe::engine::render {

struct RenderCore::Impl {
  GraphicsCore* gc = nullptr;
};

RenderCore::RenderCore() : System("RenderCore"), m_pImpl(stl::make_unique<Impl>()) {}
RenderCore::~RenderCore() {}

bool RenderCore::init(Visitor<WorldBase>& visitor) {
  auto& gc = d()->gc;
  gc = Allocator::create<GraphicsCore>();

  GCInitInfo gcInitInfo;
  gc->Initialize(gcInitInfo);

  visitor->get_context<GraphicsCore>().create(gc, false);

  // m_world->register_context<GraphicsCore>(d()->gc);

  // auto Pass = Pass::create<InitCore>([](ContextWriter<GraphicsCore> conWriter) {
  //   conWriter.create();

  //   auto& gc = conWriter.get();
  // });

  return true;
}

}  // namespace fe::engine::render