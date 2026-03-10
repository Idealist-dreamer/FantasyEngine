#include "renderGLTF.h"

#include "engine/render/shared/graphicsCore.h"
#include "engine/render/shared/stage.h"
#include "engine/render/shared/renderGltfCore.h"
#include "engine/render/shared/gltfComponents.h"
#include "engine/render/shared/renderResources.h"
#include "engine/render/shared/windowContext.h"
#include "engine/render/rhi/CommandContext.h"
#include "engine/render/rhi/GraphicsCore.h"
#include "engine/render/rhi/Camera.h"
#include "engine/render/rhi/VectorMath.h"
#include "engine/ecs/pass.h"
#include "engine/ecs/paramTypes.h"
#include "engine/base/log/log.h"

#include <DirectXMath.h>

namespace fe::engine::render {

using namespace Math;

// Per-frame constants for camera matrix
__declspec(align(16)) struct PerFrameConstants {
  DirectX::XMFLOAT4X4 viewMatrix;
  DirectX::XMFLOAT4X4 projMatrix;
  DirectX::XMFLOAT4X4 viewProjMatrix;
  DirectX::XMFLOAT3 cameraPosition;
  float _padding;
};

struct RenderGLTF::Impl {
  RenderGltfCore gltfRenderer;
  Camera camera;
  PerFrameConstants frameConstants;
  bool shadersLoaded = false;
};

RenderGLTF::RenderGLTF() : System("RenderGLTF"), m_pImpl(stl::make_unique<Impl>()) {}
RenderGLTF::~RenderGLTF() = default;

bool RenderGLTF::init(WorldVisitor& visitor) {
  // 通过 WorldVisitor 获取 WindowContext 初始化相机
  auto& windowCtx = visitor.get_context<WindowContext>();
  uint32_t width = 1280, height = 720;
  if (windowCtx.valid()) {
    width = windowCtx.get<WindowContext>()->width;
    height = windowCtx.get<WindowContext>()->height;
  }

  // Set camera position using Math::Vector3
  d()->camera.SetEyeAtUp(
    Vector3(0.0f, 2.0f, -5.0f),  // Eye position
    Vector3(0.0f, 0.0f, 0.0f),   // Look at
    Vector3(0.0f, 1.0f, 0.0f)    // Up vector
  );
  d()->camera.Update();

  // Main render pass
  m_passes.push_back(Pass::create_update<stage::Update>(
    "RenderGLTF_Draw",
    [this](ContextReader<GraphicsCore> gcReader,
           ComponentReader<GltfModelComponent> modelReader,
           ContextReader<WindowContext> windowReader) {

      if (!gcReader.valid()) {
        FE_LOG_WARN("RenderGLTF: GraphicsCore not available");
        return;
      }

      auto* graphicsCore = gcReader.get();
      GraphicsContext* gfxContext = graphicsCore->GetGraphicsContext();
      if (!gfxContext) {
        FE_LOG_WARN("RenderGLTF: GraphicsContext not available");
        return;
      }

      auto& resources = RenderResources::Instance();
      if (!resources.IsInitialized()) {
        FE_LOG_WARN("RenderGLTF: RenderResources not initialized");
        return;
      }

      // Update camera
      auto* window = windowReader.valid() ? windowReader.get() : nullptr;
      uint32_t w = window ? window->width : 1280;
      uint32_t h = window ? window->height : 720;

      auto& impl = *d();
      impl.camera.SetAspectRatio(static_cast<float>(h) / static_cast<float>(w));
      impl.camera.Update();

      // Update per-frame constants
      // Convert Math::Matrix4 to DirectX::XMFLOAT4X4
      const Matrix4& view = impl.camera.GetViewMatrix();
      const Matrix4& proj = impl.camera.GetProjMatrix();
      const Matrix4& viewProj = impl.camera.GetViewProjMatrix();

      DirectX::XMStoreFloat4x4(&impl.frameConstants.viewMatrix, view);
      DirectX::XMStoreFloat4x4(&impl.frameConstants.projMatrix, proj);
      DirectX::XMStoreFloat4x4(&impl.frameConstants.viewProjMatrix, viewProj);

      Vector3 camPos = impl.camera.GetPosition();
      impl.frameConstants.cameraPosition = DirectX::XMFLOAT3(camPos.GetX(), camPos.GetY(), camPos.GetZ());

      // Set pipeline state
      gfxContext->SetPipelineState(*resources.GetGltfPSO());
      gfxContext->SetRootSignature(*resources.GetGltfRootSignature());
      gfxContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

      // Set per-frame constants (slot 2)
      gfxContext->SetDynamicConstantBufferView(2, sizeof(PerFrameConstants), &impl.frameConstants);

      // Render all entities with GltfModelComponent
      for (auto entity : modelReader.view<GltfModelComponent>()) {
        const auto& modelComp = modelReader.get<GltfModelComponent>(entity);

        if (modelComp.modelId == 0xFFFFFFFF) continue;

        // Render the model
        impl.gltfRenderer.render(gfxContext, modelComp.modelId);
      }
    },
    uint32_t(Priority::Mid)
  ));

  return true;
}

}  // namespace fe::engine::render
