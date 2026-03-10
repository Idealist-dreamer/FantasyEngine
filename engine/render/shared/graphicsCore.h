#pragma once

#include "framework/system.h"

#include "render/rhi/GraphicsCore.h"
#include "render/rhi/SystemTime.h"
#include "render/rhi/TextRenderer.h"
#include "render/rhi/CommandContext.h"
#include "render/rhi/RootSignature.h"
#include "render/rhi/PipelineState.h"
#include "render/rhi/BufferManager.h"

namespace fe::engine::render {
struct GCInitInfo {
  bool enableDebugLayer = true;
  bool enableGPUBasedValidation = true;
  bool enableDXGIDebugInfo = true;
  bool useWarpDriver = false;
  bool requireDXRSupport = false;
  D3D_FEATURE_LEVEL d3DFeatureLevel = D3D_FEATURE_LEVEL_11_0;
  bool setStablePowerState = true;

  int swapChainBufferCount = 3;
  int swapChainWidth = 1280;
  int swapChainHeight = 720;
  DXGI_FORMAT swapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
  DXGI_SWAP_EFFECT swapChainEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  DXGI_SCALING swapChainScaling = DXGI_SCALING_NONE;
  UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
  HWND swapChainHWND;
};

using GraphicsContext = ::GraphicsContext;

// NOTE: GraphicsCore is the singleton wrapper for RHI graphics device management.
// It owns and manages the global D3D12 device and command context lifecycle.
// Do NOT create duplicate Device instances - use GetDevice() to access the single global device.
// This ensures proper resource lifetime management and prevents device lost scenarios.
class GraphicsCore {
 public:
  GraphicsCore();
  virtual ~GraphicsCore();

  void Initialize(GCInitInfo info);
  void Release();

  FE_FINLINE ID3D12Device* GetDevice() { return m_Device.Get(); }
  FE_FINLINE GraphicsContext* GetGraphicsContext() { return m_GraphicsContext; }

  bool IsDeviceNvidia();
  bool IsDeviceAMD();
  bool IsDeviceIntel();

  void SetResourcePath(const stl::wstring& assertPath) { m_AssetsPath = assertPath; }
  stl::wstring GetResourceFilePath(const stl::wstring& assetName);

  void Begin();
  void End();

  void Resize(int width, int height);

 private:
  Microsoft::WRL::ComPtr<ID3D12Device> m_Device;

  D3D_FEATURE_LEVEL m_D3DFeatureLevel;
  bool m_bTypedUAVLoadSupport_R11G11B10_FLOAT{false};
  bool m_bTypedUAVLoadSupport_R16G16B16A16_FLOAT{false};

  GraphicsContext* m_GraphicsContext = nullptr;

  stl::wstring m_AssetsPath;
};

}  // namespace fe::engine::render