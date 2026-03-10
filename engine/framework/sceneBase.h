#pragma once

#include "common.h"
#include "paramTypes.h"

namespace fe::engine {

class SceneBase {
 public:
  SceneBase() = default;
  virtual ~SceneBase() = default;

  Registry m_registry;

  stl::unordered_map<std::type_index, Any> m_context_manager;

  stl::unordered_map<std::type_index, Any> m_event_manager1;
  stl::unordered_map<std::type_index, Any> m_event_manager2;
  stl::unordered_map<std::type_index, void (*)(SceneBase&)> m_event_swap;

  stl::unordered_map<uint32_t, EntityCommandBuffer> m_entity_command_buffers;
};

}  // namespace fe::engine
