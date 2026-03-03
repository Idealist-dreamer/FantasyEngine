#pragma once

namespace fe::engine::ecs {
// Add stages within this namespace
namespace stage {
struct PreStartup {};
struct Startup {};
struct PostStartup {};

struct PreUpdate {};
struct Update {};
struct PostUpdate {};

struct Cleanup {};
}  // namespace stage
}  // namespace fe::engine::ecs