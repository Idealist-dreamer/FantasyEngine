// #include "core.h"

// #include "render/shared/graphicsCore.h"
// #include "render/shared/stage.h"
// #include "render/shared/windowContext.h"
// #include "render/shared/renderResources.h"
// #include "framework/pass.h"
// #include "core/log/log.h"

// namespace fe::engine::render {

// struct RenderCore::Impl {
//   GraphicsCore* gc = nullptr;
// };

// RenderCore::RenderCore() : System("RenderCore"), m_pImpl(stl::make_unique<Impl>()) {}
// RenderCore::~RenderCore() {}

// bool RenderCore::init(SceneBase& scene) {
//   auto& gc = d()->gc;
//   gc = Allocator::create<GraphicsCore>();

//   // 通过 System 访问器获取 WindowContext
//   auto& windowCtxStorage = get_context<WindowContext>(scene);
//   if (!windowCtxStorage.valid()) {
//     FE_LOG_ERROR("RenderCore: WindowContext not found in SceneBase!");
//     return false;
//   }

//   WindowContext* pWindow = windowCtxStorage.get<WindowContext>();

//   // Initialize GraphicsCore with window info
//   GCInitInfo gcInitInfo;
//   gcInitInfo.swapChainHWND = pWindow->hwnd;
//   gcInitInfo.swapChainWidth = pWindow->width;
//   gcInitInfo.swapChainHeight = pWindow->height;
//   gcInitInfo.enableDebugLayer = true;
//   gcInitInfo.enableGPUBasedValidation = false;  // Disable for performance
//   gcInitInfo.enableDXGIDebugInfo = true;

//   gc->Initialize(gcInitInfo);

//   // Register GraphicsCore to context
//   get_context<GraphicsCore>(scene).create(gc, false);

//   // Initialize render resources (PSO, RootSignature, etc.)
//   RenderResources::Instance().Initialize();

//   // Add render frame pass
//   m_passes.push_back(Pass::create_update<stage::Update>(
//       "RenderFrame",
//       [](ContextWriter<GraphicsCore> gcReader, ContextWriter<WindowContext> windowReader) {
//         if (!gcReader.valid())
//           return;

//         GraphicsCore& graphicsCore = gcReader.get();

//         if (windowReader.valid()) {
//           WindowContext& window = windowReader.get();
//           if (window.resized) {
//             graphicsCore.Resize(window.width, window.height);
//             window.resized = false;
//           }
//         }

//         // Begin frame
//         graphicsCore.Begin();
//       },
//       uint32_t(Priority::First)));

//   // Add end frame pass
//   m_passes.push_back(Pass::create_update<stage::Last>(
//       "EndFrame",
//       [](ContextWriter<GraphicsCore> gcReader) {
//         if (!gcReader.valid())
//           return;
//         gcReader.get().End();
//       },
//       uint32_t(Priority::Low)));

//   return true;
// }

// }  // namespace fe::engine::render
