// #include "renderResources.h"

// #include "render/rhi/GraphicsCore.h"
// #include "render/rhi/GraphicsCommon.h"
// #include "render/rhi/CommandContext.h"
// #include "core/log/log.h"

// #include <d3dcompiler.h>

// #pragma comment(lib, "d3dcompiler.lib")

// namespace fe::engine::render {

// // Helper to compile shader from HLSL file
// static bool CompileShaderFromFile(const wchar_t* fileName, const char* entryPoint, const char* target, ID3DBlob** ppBlob) {
//   UINT compileFlags = 0;
// #if defined(_DEBUG)
//   compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
// #endif

//   ID3DBlob* pErrorBlob = nullptr;
//   HRESULT hr = D3DCompileFromFile(fileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
//                                    entryPoint, target, compileFlags, 0, ppBlob, &pErrorBlob);

//   if (FAILED(hr)) {
//     if (pErrorBlob) {
//       FE_LOG_ERROR("Shader compilation failed: {}", (char*)pErrorBlob->GetBufferPointer());
//       pErrorBlob->Release();
//     }
//     return false;
//   }

//   if (pErrorBlob) pErrorBlob->Release();
//   return true;
// }

// void RenderResources::Initialize() {
//   if (m_initialized) return;

//   // Create Root Signature for glTF rendering
//   // Layout:
//   // [0] CBV (b0) - BindlessDrawConstants (32 bytes = 8 DWORDs)
//   // [1] SRV (t0) - MegaBuffer (ByteAddressBuffer)
//   // [2] CBV (b1) - PerFrameConstants (for camera matrix)
//   m_gltfRootSignature = stl::make_unique<RootSignature>(3, 0);

//   // Slot 0: BindlessDrawConstants (root constants, 8 DWORDs)
//   (*m_gltfRootSignature)[0].InitAsConstants(0, 8, D3D12_SHADER_VISIBILITY_VERTEX);

//   // Slot 1: MegaBuffer SRV
//   (*m_gltfRootSignature)[1].InitAsBufferSRV(0, D3D12_SHADER_VISIBILITY_VERTEX);

//   // Slot 2: PerFrameConstants CBV (for camera/view matrix)
//   (*m_gltfRootSignature)[2].InitAsConstantBuffer(1, D3D12_SHADER_VISIBILITY_ALL);

//   m_gltfRootSignature->Finalize(L"GltfRootSignature", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

//   // Create PSO
//   m_gltfPSO = stl::make_unique<GraphicsPSO>(L"GltfPSO");
//   m_gltfPSO->SetRootSignature(*m_gltfRootSignature);

//   // Use default rasterizer/depth state
//   m_gltfPSO->SetRasterizerState(Graphics::RasterizerDefault);
//   m_gltfPSO->SetBlendState(Graphics::BlendDisable);
//   m_gltfPSO->SetDepthStencilState(Graphics::DepthStateReadWrite);

//   // Primitive topology
//   m_gltfPSO->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

//   // Render target format - use R11G11B10_FLOAT to match scene color buffer
//   // Note: g_SceneColorBuffer format is R11G11B10_FLOAT (defined in BufferManager)
//   DXGI_FORMAT rtFormat = DXGI_FORMAT_R11G11B10_FLOAT;
//   DXGI_FORMAT dsvFormat = DXGI_FORMAT_D32_FLOAT;
//   m_gltfPSO->SetRenderTargetFormats(1, &rtFormat, dsvFormat);

//   // No input layout - we use bindless vertex fetching
//   m_gltfPSO->SetInputLayout(0, nullptr);

//   // Load and compile shaders
//   ID3DBlob* vsBlob = nullptr;
//   ID3DBlob* psBlob = nullptr;

//   // Try multiple paths for shader files
//   const wchar_t* shaderPaths[] = {
//     L"resource/runtime/hlsl/GltfVS.hlsl",
//     L"../resource/runtime/hlsl/GltfVS.hlsl",
//     L"../../resource/runtime/hlsl/GltfVS.hlsl"
//   };

//   bool vsLoaded = false;
//   for (const wchar_t* path : shaderPaths) {
//     if (CompileShaderFromFile(path, "main", "vs_5_1", &vsBlob)) {
//       vsLoaded = true;
//       FE_LOG_INFO("Loaded vertex shader from: {}", (const char*)path);
//       break;
//     }
//   }

//   const wchar_t* psPaths[] = {
//     L"resource/runtime/hlsl/GltfPS.hlsl",
//     L"../resource/runtime/hlsl/GltfPS.hlsl",
//     L"../../resource/runtime/hlsl/GltfPS.hlsl"
//   };

//   bool psLoaded = false;
//   for (const wchar_t* path : psPaths) {
//     if (CompileShaderFromFile(path, "main", "ps_5_1", &psBlob)) {
//       psLoaded = true;
//       FE_LOG_INFO("Loaded pixel shader from: {}", (const char*)path);
//       break;
//     }
//   }

//   if (vsLoaded && vsBlob) {
//     m_gltfPSO->SetVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize());
//   } else {
//     FE_LOG_ERROR("Failed to load GltfVS.hlsl");
//   }

//   if (psLoaded && psBlob) {
//     m_gltfPSO->SetPixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize());
//   } else {
//     FE_LOG_ERROR("Failed to load GltfPS.hlsl");
//   }

//   // Finalize PSO
//   m_gltfPSO->Finalize();

//   // Release shader blobs
//   if (vsBlob) vsBlob->Release();
//   if (psBlob) psBlob->Release();

//   m_initialized = true;
//   FE_INFO("RenderResources initialized successfully");
// }

// void RenderResources::Shutdown() {
//   m_gltfPSO.reset();
//   m_gltfRootSignature.reset();
//   m_initialized = false;
// }

// }  // namespace fe::engine::render
