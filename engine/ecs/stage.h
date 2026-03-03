#pragma once

namespace fe::engine::ecs {
// Add stages within this namespace
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