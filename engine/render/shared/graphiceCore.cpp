#include "graphicsCore.h"

#include "render/rhi/GraphicsCore.h"
#include "render/rhi/GameCore.h"
#include "render/rhi/BufferManager.h"
#include "render/rhi/GpuTimeManager.h"
#include "render/rhi/PostEffects.h"
#include "render/rhi/SSAO.h"
#include "render/rhi/TextRenderer.h"
#include "render/rhi/ColorBuffer.h"
#include "render/rhi/SystemTime.h"
#include "render/rhi/SamplerManager.h"
#include "render/rhi/DescriptorHeap.h"
#include "render/rhi/CommandContext.h"
#include "render/rhi/CommandListManager.h"
#include "render/rhi/RootSignature.h"
#include "render/rhi/CommandSignature.h"
#include "render/rhi/ParticleEffectManager.h"
#include "render/rhi/GraphRenderer.h"
#include "render/rhi/TemporalEffects.h"
#include "render/rhi/Display.h"

#include "foundation/platform/dx12.h"

namespace GameCore {
extern HWND g_hWnd;
}

namespace fe::engine::render {
using namespace Microsoft::WRL;

static const uint32_t vendorID_Nvidia = 0x10DE;
static const uint32_t vendorID_AMD = 0x1002;
static const uint32_t vendorID_Intel = 0x8086;

const wchar_t* GPUVendorToString(uint32_t vendorID) {
  switch (vendorID) {
    case vendorID_Nvidia:
      return L"Nvidia";
    case vendorID_AMD:
      return L"AMD";
    case vendorID_Intel:
      return L"Intel";
    default:
      return L"Unknown";
      break;
  }
}

uint32_t GetVendorIdFromDevice(ID3D12Device* pDevice) {
  LUID luid = pDevice->GetAdapterLuid();

  // Obtain the DXGI factory
  Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
  FE_ASSERT_SUCCEEDED(CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory)));

  Microsoft::WRL::ComPtr<IDXGIAdapter1> pAdapter;

  FE_ASSERT_SUCCEEDED(dxgiFactory->EnumAdapterByLuid(luid, IID_PPV_ARGS(&pAdapter)));
  DXGI_ADAPTER_DESC1 desc;
  FE_ASSERT_SUCCEEDED(pAdapter->GetDesc1(&desc));

  return desc.VendorId;
}

bool GraphicsCore::IsDeviceNvidia() {
  return GetVendorIdFromDevice(m_Device.Get()) == vendorID_Nvidia;
}
bool GraphicsCore::IsDeviceAMD() {
  return GetVendorIdFromDevice(m_Device.Get()) == vendorID_AMD;
}
bool GraphicsCore::IsDeviceIntel() {
  return GetVendorIdFromDevice(m_Device.Get()) == vendorID_Intel;
}

bool IsDirectXRaytracingSupported(ID3D12Device* testDevice) {
  D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupport = {};

  if (FAILED(testDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupport, sizeof(featureSupport))))
    return false;

  return featureSupport.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
}

GraphicsCore::GraphicsCore() {}
GraphicsCore::~GraphicsCore() {
  Release();
}

void GraphicsCore::Initialize(GCInitInfo info) {
  // DebugLayers
  DWORD dxgiFactoryFlags = 0;

  if (info.enableDebugLayer) {
    Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)))) {
      debugInterface->EnableDebugLayer();

      if (info.enableGPUBasedValidation) {
        Microsoft::WRL::ComPtr<ID3D12Debug1> debugInterface1;
        if (SUCCEEDED((debugInterface->QueryInterface(IID_PPV_ARGS(&debugInterface1))))) {
          debugInterface1->SetEnableGPUBasedValidation(true);
        }
      }
    } else {
      FE_WARN_ONCE_IF(0, "WARNING:  Unable to enable D3D12 debug validation layer\n");
    }

    if (info.enableDXGIDebugInfo) {
      ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
      if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf())))) {
        dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

        dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
        dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

        DXGI_INFO_QUEUE_MESSAGE_ID hide[] = {
            80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */
            ,
        };
        DXGI_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumIDs = _countof(hide);
        filter.DenyList.pIDList = hide;
        dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
      }
    }
  }

  // Obtain the DXGI factory
  Microsoft::WRL::ComPtr<IDXGIFactory6> dxgiFactory;
  FE_ASSERT_SUCCEEDED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

  // Create the D3D graphics device
  Microsoft::WRL::ComPtr<ID3D12Device> pDevice;
  Microsoft::WRL::ComPtr<IDXGIAdapter1> pAdapter;

  // Temporary workaround because SetStablePowerState() is crashing
  D3D12EnableExperimentalFeatures(0, nullptr, nullptr, nullptr);

  if (!info.useWarpDriver) {
    SIZE_T MaxSize = 0;

    Microsoft::WRL::ComPtr<ID3D12Device> pDevice;
    for (uint32_t Idx = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapters1(Idx, &pAdapter); ++Idx) {
      DXGI_ADAPTER_DESC1 desc;
      pAdapter->GetDesc1(&desc);

      // Is a software adapter?
      if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        continue;

      // Can create a D3D12 device?
      if (FAILED(D3D12CreateDevice(pAdapter.Get(), info.d3DFeatureLevel, IID_PPV_ARGS(&pDevice))))
        continue;

      // Does support DXR if required?
      if (info.requireDXRSupport && !IsDirectXRaytracingSupported(pDevice.Get()))
        continue;

      // By default, search for the adapter with the most memory because that's usually the dGPU.
      if (desc.DedicatedVideoMemory < MaxSize)
        continue;

      MaxSize = desc.DedicatedVideoMemory;

      if (m_Device != nullptr) {
        m_Device->Release();
      }

      m_Device = pDevice.Detach();

      wprintf(L"Selected GPU:  %ls (%llu MB)\n", desc.Description, desc.DedicatedVideoMemory >> 20);
    }
  } else {
    FE_ASSERT_SUCCEEDED(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pAdapter)));
    FE_ASSERT_SUCCEEDED(D3D12CreateDevice(pAdapter.Get(), info.d3DFeatureLevel, IID_PPV_ARGS(&m_Device)));
  }

  if (info.requireDXRSupport && !IsDirectXRaytracingSupported(m_Device.Get())) {
    printf("Unable to find a DXR-capable device. Halting.\n");
    __debugbreak();
  }

  if (m_Device == nullptr) {
    FE_ASSERT(0, "Create device error\n");
  } else if (info.setStablePowerState) {
    bool DeveloperModeEnabled = false;

    // Look in the Windows Registry to determine if Developer Mode is enabled
    HKEY hKey;
    LSTATUS result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock", 0, KEY_READ, &hKey);
    if (result == ERROR_SUCCESS) {
      DWORD keyValue, keySize = sizeof(DWORD);
      result = RegQueryValueExW(hKey, L"AllowDevelopmentWithoutDevLicense", 0, NULL, (byte*)&keyValue, &keySize);
      if (result == ERROR_SUCCESS && keyValue == 1)
        DeveloperModeEnabled = true;
      RegCloseKey(hKey);
    }

    FE_WARN_ONCE_IF_NOT(DeveloperModeEnabled, "Enable Developer Mode on Windows 10 to get consistent profiling results");

    // Prevent the GPU from overclocking or underclocking to get consistent timings
    if (DeveloperModeEnabled) {
      // m_Device->SetStablePowerState(TRUE);
    }
  }

  Graphics::g_Device = m_Device.Get();
  m_D3DFeatureLevel = info.d3DFeatureLevel;

  if (info.enableDebugLayer) {
    ID3D12InfoQueue* pInfoQueue = nullptr;
    if (SUCCEEDED(m_Device->QueryInterface(IID_PPV_ARGS(&pInfoQueue)))) {
      // Suppress whole categories of messages
      //D3D12_MESSAGE_CATEGORY Categories[] = {};

      // Suppress messages based on their severity level
      D3D12_MESSAGE_SEVERITY Severities[] = {D3D12_MESSAGE_SEVERITY_INFO};

      // Suppress individual messages by their ID
      D3D12_MESSAGE_ID DenyIds[] = {
          // This occurs when there are uninitialized descriptors in a descriptor table, even when a
          // shader does not access the missing descriptors.  I find this is common when switching
          // shader permutations and not wanting to change much code to reorder resources.
          D3D12_MESSAGE_ID_INVALID_DESCRIPTOR_HANDLE,

          // Triggered when a shader does not export all color components of a render target, such as
          // when only writing RGB to an R10G10B10A2 buffer, ignoring alpha.
          D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_PS_OUTPUT_RT_OUTPUT_MISMATCH,

          // This occurs when a descriptor table is unbound even when a shader does not access the missing
          // descriptors.  This is common with a root signature shared between disparate shaders that
          // don't all need the same types of resources.
          D3D12_MESSAGE_ID_COMMAND_LIST_DESCRIPTOR_TABLE_NOT_SET,

          // RESOURCE_BARRIER_DUPLICATE_SUBRESOURCE_TRANSITIONS
          D3D12_MESSAGE_ID_RESOURCE_BARRIER_DUPLICATE_SUBRESOURCE_TRANSITIONS,

          // Suppress errors from calling ResolveQueryData with timestamps that weren't requested on a given frame.
          D3D12_MESSAGE_ID_RESOLVE_QUERY_INVALID_QUERY_STATE,

          // Ignoring InitialState D3D12_RESOURCE_STATE_COPY_DEST. Buffers are effectively created in state D3D12_RESOURCE_STATE_COMMON.
          D3D12_MESSAGE_ID_CREATERESOURCE_STATE_IGNORED,
      };

      D3D12_INFO_QUEUE_FILTER NewFilter = {};
      //NewFilter.DenyList.NumCategories = _countof(Categories);
      //NewFilter.DenyList.pCategoryList = Categories;
      NewFilter.DenyList.NumSeverities = _countof(Severities);
      NewFilter.DenyList.pSeverityList = Severities;
      NewFilter.DenyList.NumIDs = _countof(DenyIds);
      NewFilter.DenyList.pIDList = DenyIds;

      pInfoQueue->PushStorageFilter(&NewFilter);
      pInfoQueue->Release();
    }
  }

  // We like to do read-modify-write operations on UAVs during post processing.  To support that, we
  // need to either have the hardware do typed UAV loads of R11G11B10_FLOAT or we need to manually
  // decode an R32_UINT representation of the same buffer.  This code determines if we get the hardware
  // load support.
  D3D12_FEATURE_DATA_D3D12_OPTIONS FeatureData = {};
  if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &FeatureData, sizeof(FeatureData)))) {
    if (FeatureData.TypedUAVLoadAdditionalFormats) {
      D3D12_FEATURE_DATA_FORMAT_SUPPORT Support = {DXGI_FORMAT_R11G11B10_FLOAT, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE};

      if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &Support, sizeof(Support))) &&
          (Support.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0) {
        m_bTypedUAVLoadSupport_R11G11B10_FLOAT = true;
      }

      Support.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

      if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &Support, sizeof(Support))) &&
          (Support.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0) {
        m_bTypedUAVLoadSupport_R16G16B16A16_FLOAT = true;
      }
    }
  }

  // config
  {
    Graphics::g_SceneColorBuffer.SetClearColor({0.24, 0.24, 0.24, 1.0f});
    Graphics::g_DisplayWidth = info.swapChainWidth;
    Graphics::g_DisplayHeight = info.swapChainHeight;
  }

  Graphics::g_CommandManager.Create(m_Device.Get());

  Graphics::InitializeCommonState();

  GameCore::g_hWnd = info.swapChainHWND;
  Display::Initialize();

  GpuTimeManager::Initialize(4096);
  TemporalEffects::Initialize();
  PostEffects::Initialize();
  SSAO::Initialize();
  TextRenderer::Initialize();
  GraphRenderer::Initialize();
  ParticleEffectManager::Initialize(3840, 2160);
}

stl::wstring GraphicsCore::GetResourceFilePath(const stl::wstring& assetName) {
  return m_AssetsPath + assetName;
}

void GraphicsCore::Begin() {
  GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Render");

  gfxContext.TransitionResource(Graphics::g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
  gfxContext.TransitionResource(Graphics::g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);

  gfxContext.ClearColor(Graphics::g_SceneColorBuffer);
  gfxContext.ClearDepth(Graphics::g_SceneDepthBuffer);

  gfxContext.SetRenderTargets(1, &Graphics::g_SceneColorBuffer.GetRTV(), Graphics::g_SceneDepthBuffer.GetDSV());
  gfxContext.SetViewportAndScissor(0, 0, Graphics::g_SceneColorBuffer.GetWidth(), Graphics::g_SceneColorBuffer.GetHeight());

  m_GraphicsContext = &gfxContext;
}
void GraphicsCore::End() {
  m_GraphicsContext->Finish();

  PostEffects::Render();
  Display::Present();

  m_GraphicsContext = nullptr;
}

void GraphicsCore::Release() {
  Graphics::g_CommandManager.IdleGPU();

  CommandContext::DestroyAllContexts();
  Graphics::g_CommandManager.Shutdown();
  GpuTimeManager::Shutdown();
  PSO::DestroyAll();
  RootSignature::DestroyAll();
  DescriptorAllocator::DestroyAll();

  Graphics::DestroyCommonState();
  Graphics::DestroyRenderingBuffers();
  TemporalEffects::Shutdown();
  PostEffects::Shutdown();
  SSAO::Shutdown();
  TextRenderer::Shutdown();
  GraphRenderer::Shutdown();
  ParticleEffectManager::Shutdown();
  Display::Shutdown();

#if defined(_GAMING_DESKTOP) && defined(_DEBUG)
  ID3D12DebugDevice* debugInterface;
  if (SUCCEEDED(g_Device->QueryInterface(&debugInterface))) {
    debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
    debugInterface->Release();
  }
#endif

  if (Graphics::g_Device != nullptr) {
    m_Device = nullptr;
    Graphics::g_Device = nullptr;
  }
}

void GraphicsCore::Resize(int width, int height) {
  Display::Resize(width, height);
}

}  // namespace fe::engine::render