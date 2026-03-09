#pragma once

#include "common.h"
#include "context.h"
#include "asset.h"

#include "paramTypes.h"
#include "paramTraits.h"
#include "paramAccess.h"

namespace fe::engine {

class WorldBase {
 public:
  WorldBase() = default;
  virtual ~WorldBase() = default;

  template <typename T>
  ContextStorage& get_context() {
    return m_context_manager[typeid(T)];
  }

 protected:
  Registry m_registry;

  stl::unordered_map<std::type_index, ContextStorage> m_context_manager;

  stl::unordered_map<std::type_index, ContextStorage> m_event_manager1;
  stl::unordered_map<std::type_index, ContextStorage> m_event_manager2;
  stl::unordered_map<std::type_index, void (*)(WorldBase&)> m_event_swap;

  stl::unordered_map<uint32_t, EntityCommandBuffer> m_entity_command_buffers;

  friend class Visitor<WorldBase>;
};

}  // namespace fe::engine
