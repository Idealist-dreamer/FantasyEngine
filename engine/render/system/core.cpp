#include "core.h"

#include "render/shared/graphicsCore.h"
#include "render/shared/stage.h"
#include "render/shared/window.h"

namespace fe::engine::render {

struct RenderCore::Impl {
  GraphicsCore* gc = nullptr;
};

RenderCore::RenderCore() : System("RenderCore"), m_pImpl(stl::make_unique<Impl>()) {}
RenderCore::~RenderCore() {}

bool RenderCore::init(SceneBase& scene) {
  auto& gc = d()->gc;
  gc = Allocator::create<GraphicsCore>();

  // Initialize GraphicsCore with window info
  GCInitInfo gcInitInfo;
  //   gcInitInfo.swapChainHWND = pWindow->hwnd;
  //   gcInitInfo.swapChainWidth = pWindow->width;
  //   gcInitInfo.swapChainHeight = pWindow->height;
  //   gcInitInfo.enableDebugLayer = true;
  //   gcInitInfo.enableGPUBasedValidation = false;  // Disable for performance
  gcInitInfo.enableDXGIDebugInfo = true;

  gc->Initialize(gcInitInfo);

  return true;
}

}  // namespace fe::engine::render
