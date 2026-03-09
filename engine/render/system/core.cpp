#include "core.h"

#include "engine/render/shared/graphicsCore.h"
#include "engine/render/shared/stage.h"
#include "engine/render/shared/windowContext.h"
#include "engine/render/shared/renderResources.h"
#include "engine/ecs/pass.h"
#include "engine/base/log/log.h"

namespace fe::engine::render {

struct RenderCore::Impl {
  GraphicsCore* gc = nullptr;
};

RenderCore::RenderCore() : System("RenderCore"), m_pImpl(stl::make_unique<Impl>()) {}
RenderCore::~RenderCore() {}

bool RenderCore::init(Visitor<WorldBase>& visitor) {
  auto& gc = d()->gc;
  gc = Allocator::create<GraphicsCore>();
  
  // Get window context from WorldBase
  auto& windowCtxStorage = visitor->get_context<WindowContext>();
  if (!windowCtxStorage.valid()) {
    FE_LOG_ERROR("RenderCore: WindowContext not found in WorldBase!");
    return false;
  }
  
  WindowContext* pWindow = windowCtxStorage.get<WindowContext>();
  
  // Initialize GraphicsCore with window info
  GCInitInfo gcInitInfo;
  gcInitInfo.swapChainHWND = pWindow->hwnd;
  gcInitInfo.swapChainWidth = pWindow->width;
  gcInitInfo.swapChainHeight = pWindow->height;
  gcInitInfo.enableDebugLayer = true;
  gcInitInfo.enableGPUBasedValidation = false;  // Disable for performance
  gcInitInfo.enableDXGIDebugInfo = true;
  
  gc->Initialize(gcInitInfo);
  
  // Register GraphicsCore to context
  visitor->get_context<GraphicsCore>().create(gc, false);
  
  // Initialize render resources (PSO, RootSignature, etc.)
  RenderResources::Instance().Initialize();
  
  // Add render frame pass
  m_passes.push_back(Pass::create_update<stage::Update>(
    "RenderFrame",
    [](ContextReader<GraphicsCore> gcReader, ContextReader<WindowContext> windowReader) {
      if (!gcReader.valid()) return;
      
      GraphicsCore* graphicsCore = gcReader.get();
      if (!graphicsCore) return;
      
      if (windowReader.valid()) {
        WindowContext* window = windowReader.get();
        if (window && window->resized) {
          graphicsCore->Resize(window->width, window->height);
          window->resized = false;
        }
      }
      
      // Begin frame
      graphicsCore->Begin();
    },
    uint32_t(Priority::First)
  ));
  
  // Add end frame pass
  m_passes.push_back(Pass::create_update<stage::Last>(
    "EndFrame",
    [](ContextReader<GraphicsCore> gcReader) {
      if (!gcReader.valid()) return;
      gcReader.get()->End();
    },
    uint32_t(Priority::Last)
  ));
  
  return true;
}

}  // namespace fe::engine::render
