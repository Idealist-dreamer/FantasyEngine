#pragma once

namespace fe::engine::ecs::stage {
struct Startup {};
struct PreStartup {};
struct PostStartup {};

struct Update {};
struct PreUpdate {};
struct PostUpdate {};

struct Cleanup {};
}  // namespace fe::engine::ecs::stage