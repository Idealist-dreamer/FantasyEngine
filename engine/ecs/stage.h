#pragma once

namespace fe::engine::ecs {
namespace stage {
struct Startup {};
struct PreStartup {};
struct PostStartup {};

struct Update {};
struct PreUpdate {};
struct PostUpdate {};

struct Cleanup {};
}  // namespace stage
}  // namespace fe::engine::ecs