#pragma once

#include "render/rhi/RootSignature.h"
#include "render/rhi/PipelineState.h"
#include "core/container/stl.h"

namespace fe::engine::render {

/// Shared render resources for glTF rendering
/// Manages PSO, RootSignature, and other GPU resources
class RenderResources {
 public:
  static RenderResources& Instance() {
    static RenderResources instance;
    return instance;
  }

  void Initialize();
  void Shutdown();

  RootSignature* GetGltfRootSignature() { return m_gltfRootSignature.get(); }
  GraphicsPSO* GetGltfPSO() { return m_gltfPSO.get(); }

  bool IsInitialized() const { return m_initialized; }

 private:
  RenderResources() = default;
  ~RenderResources() { Shutdown(); }

  bool m_initialized = false;
  stl::unique_ptr<RootSignature> m_gltfRootSignature;
  stl::unique_ptr<GraphicsPSO> m_gltfPSO;
};

}  // namespace fe::engine::render
