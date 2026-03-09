#pragma once

#include <Windows.h>

namespace fe::engine::render {

/// Window context for DX12 swap chain initialization
/// Must be set in WorldBase context before RenderCore initialization
struct WindowContext {
  HWND hwnd = nullptr;
  uint32_t width = 1280;
  uint32_t height = 720;
  bool resized = false;
  
  WindowContext() = default;
  WindowContext(HWND h, uint32_t w, uint32_t h_) : hwnd(h), width(w), height(h_) {}
};

}  // namespace fe::engine::render
