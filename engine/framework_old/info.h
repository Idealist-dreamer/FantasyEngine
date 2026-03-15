#pragma once

namespace fe::engine {

struct FrameInfo {
  float delta_time{0.0f};
  float total_time{0.0f};
  uint32_t frame_count{0};
};

}  // namespace fe::engine