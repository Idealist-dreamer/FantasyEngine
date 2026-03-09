//=============================================================================
// FantasyEngine - glTF Rendering Demo
// 
// Demonstrates ECS-based glTF model rendering with DirectX 12
//=============================================================================

#include <Windows.h>
#include <chrono>
#include <random>
#include <string>

#include "engine/ecs/world.h"
#include "engine/ecs/system.h"
#include "engine/ecs/pass.h"
#include "engine/ecs/stage.h"
#include "engine/ecs/paramTypes.h"
#include "engine/base/log/log.h"
#include "engine/base/utility/timer.h"

#include "engine/render/shared/graphicsCore.h"
#include "engine/render/shared/windowContext.h"
#include "engine/render/shared/gltfComponents.h"
#include "engine/render/shared/renderGltfCore.h"
#include "engine/render/shared/renderResources.h"
#include "engine/render/shared/stage.h"

#include "engine/render/system/core.h"
#include "engine/render/system/renderGLTF.h"

#include <DirectXMath.h>

using namespace fe::engine;
using namespace fe::engine::render;
using namespace DirectX;

//-----------------------------------------------------------------------------
// Window Creation
//-----------------------------------------------------------------------------
static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    case WM_SIZE: {
      // Signal resize through WindowContext
      // This would require access to the World, handled separately
      break;
    }
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}

static HWND CreateMainWindow(HINSTANCE hInstance, int width, int height, const wchar_t* title) {
  WNDCLASSEXW wcex = {};
  wcex.cbSize = sizeof(WNDCLASSEXW);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = WindowProc;
  wcex.hInstance = hInstance;
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.lpszClassName = L"FantasyEngineWindow";
  RegisterClassExW(&wcex);

  RECT rc = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
  AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

  HWND hwnd = CreateWindowExW(
    0, L"FantasyEngineWindow", title,
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, CW_USEDEFAULT,
    rc.right - rc.left, rc.bottom - rc.top,
    nullptr, nullptr, hInstance, nullptr);

  ShowWindow(hwnd, SW_SHOW);
  return hwnd;
}

//-----------------------------------------------------------------------------
// Test Data Setup System
//-----------------------------------------------------------------------------
class TestDataSystem : public System {
public:
  TestDataSystem() : System("TestDataSystem") {}
  
  bool init(Visitor<WorldBase>& visitor) override {
    // Add startup pass to create test entities
    m_passes.push_back(Pass::create_start<stage::Startup>(
      "CreateTestEntities",
      [](EntityCreator creator,
         ComponentWriter<TransformComponent, GltfModelComponent> writer,
         ContextWriter<RenderGltfCore> gltfContext) {
        
        // Create RenderGltfCore if not exists
        if (!gltfContext.valid()) {
          gltfContext.create();
        }
        
        auto& gltfRenderer = gltfContext.get();
        
        // Load a test glTF model (would need actual file)
        // For now, just create placeholder entities
        
        FE_LOG_INFO("Creating test entities...");
        
        // Create a few test entities with transforms
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> posDist(-5.0f, 5.0f);
        std::uniform_real_distribution<float> scaleDist(0.5f, 2.0f);
        
        for (int i = 0; i < 5; ++i) {
          Entity e = creator.create();
          
          // Add transform
          TransformComponent transform;
          transform.position = {posDist(rng), posDist(rng), posDist(rng)};
          transform.scale = {scaleDist(rng), scaleDist(rng), scaleDist(rng)};
          writer.add<TransformComponent>(e, transform);
          
          // Add glTF model component with placeholder model ID
          // In real usage, would load actual glTF file
          GltfModelComponent modelComp;
          modelComp.modelId = 0xFFFFFFFF;  // Placeholder - no model loaded
          XMMATRIX world = transform.GetWorldMatrix();
          modelComp.worldMatrix;
          XMStoreFloat4x4(&modelComp.worldMatrix, world);
          writer.add<GltfModelComponent>(e, modelComp);
        }
        
        FE_LOG_INFO("Created 5 test entities");
      },
      uint32_t(Priority::Mid)
    ));
    
    return true;
  }
};

//-----------------------------------------------------------------------------
// Main Entry Point
//-----------------------------------------------------------------------------
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int nCmdShow) {
  // Initialize logging
  LogManager::Instance().Initialize("FantasyEngine");
  FE_LOG_INFO("==========================================");
  FE_LOG_INFO("  FantasyEngine - glTF Rendering Demo");
  FE_LOG_INFO("==========================================");
  
  try {
    // Create window
    const uint32_t windowWidth = 1280;
    const uint32_t windowHeight = 720;
    HWND hwnd = CreateMainWindow(hInstance, windowWidth, windowHeight, L"FantasyEngine");
    if (!hwnd) {
      FE_LOG_ERROR("Failed to create window");
      return -1;
    }
    
    // Create ECS World
    World world;
    
    // Setup window context BEFORE initializing render systems
    auto& windowCtx = world.get_context<WindowContext>();
    windowCtx.create<WindowContext>(hwnd, windowWidth, windowHeight);
    
    // Register context for RenderGltfCore (shared across systems)
    auto& gltfCtx = world.get_context<RenderGltfCore>();
    
    // Add systems
    world.add_system(stl::make_shared<RenderCore>());
    world.add_system(stl::make_shared<TestDataSystem>());
    world.add_system(stl::make_shared<RenderGLTF>());
    
    // Compile execution graph
    FE_LOG_INFO("Compiling execution graph...");
    world.compile();
    
    // Setup (runs Init and Startup stages)
    FE_LOG_INFO("Running setup phase...");
    world.setup();
    
    // Main loop
    FE_LOG_INFO("Entering main loop...");
    
    auto lastTime = std::chrono::high_resolution_clock::now();
    bool running = true;
    
    while (running) {
      // Process Windows messages
      MSG msg = {};
      while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
          running = false;
          break;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
      
      if (!running) break;
      
      // Calculate delta time
      auto currentTime = std::chrono::high_resolution_clock::now();
      float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
      lastTime = currentTime;
      
      // Run ECS update
      world.run();
    }
    
    FE_LOG_INFO("Shutting down...");
    
    // Cleanup
    RenderResources::Instance().Shutdown();
    LogManager::Instance().Shutdown();
    
  } catch (const std::exception& e) {
    FE_LOG_ERROR("Exception: {}", e.what());
    LogManager::Instance().Shutdown();
    return -1;
  }
  
  return 0;
}
