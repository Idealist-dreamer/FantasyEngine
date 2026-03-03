#pragma once

#include "common.h"
#include "resource.h"

#include "accessEntity.h"
#include "accessComponent.h"
#include "accessEvent.h"

namespace fe::engine::ecs {
template <typename T>
concept IsEntityQuery = is_entity_query<std::remove_cvref_t<T>>::value;
template <typename T>
concept IsEntityCreator = is_entity_creator<std::remove_cvref_t<T>>::value;
template <typename T>
concept IsEntityDestroyer = is_entity_destroyer<std::remove_cvref_t<T>>::value;
template <typename T>
concept IsEntityCommandBuffer = is_entity_command_buffer<std::remove_cvref_t<T>>::value;

template <typename T>
concept IsComponentReader = is_component_reader<std::remove_cvref_t<T>>::value;
template <typename T>
concept IsComponentWriter = is_component_writer<std::remove_cvref_t<T>>::value;

template <typename T>
concept IsEventReader = is_event_reader<std::remove_cvref_t<T>>::value;
template <typename T>
concept IsEventWriter = is_event_writer<std::remove_cvref_t<T>>::value;

template <typename T>
concept IsResourceParam = is_resource_reader<T>::value || is_resource_writer<T>::value;

class WorldBase {
 public:
  WorldBase() = default;
  virtual ~WorldBase() = default;

  template <typename Component, auto Candidate>
  void on_construct() {
    m_registry.on_construct<Component>().template connect<Candidate>();
  }

  template <typename Component, auto Candidate>
  void on_update() {
    m_registry.on_update<Component>().template connect<Candidate>();
  }

  template <typename Component, auto Candidate>
  void on_destroy() {
    m_registry.on_destroy<Component>().template connect<Candidate>();
  }

  template <typename Component, auto Candidate, typename Type>
  void on_destroy(Type& instance) {
    m_registry.on_destroy<Component>().template connect<Candidate>(instance);
  }

 protected:
  template <IsEntityQuery EQ>
  auto get_param(uint32_t passId) {
    return std::remove_cvref_t<EQ>(m_registry);
  }
  template <IsEntityCreator EC>
  auto get_param(uint32_t passId) {
    return std::remove_cvref_t<EC>(m_registry);
  }
  template <IsEntityDestroyer ED>
  auto get_param(uint32_t passId) {
    return std::remove_cvref_t<ED>(m_registry);
  }
  template <IsEntityCommandBuffer EB>
  auto get_param(uint32_t passId) {
    return m_entity_command_buffers[passId];
  }

  template <IsComponentReader CR>
  auto get_param(uint32_t passId) {
    return std::remove_cvref_t<CR>(m_registry);
  }
  template <IsComponentWriter CW>
  auto get_param(uint32_t passId) {
    return std::remove_cvref_t<CW>(m_registry);
  }

  template <IsEventReader ER>
  auto get_param(uint32_t passId) {
    using T = typename is_event_reader<std::remove_cvref_t<ER>>::type;
    auto it = m_event_manager1.find(std::type_index(typeid(T)));
    FE_ASSERT(it != m_event_manager1.end() && "Event type not registered!");
    return std::remove_cvref_t<ER>(*it->second.get<T>());
  }
  template <IsEventWriter EW>
  auto get_param(uint32_t passId) {
    using T = typename is_event_writer<std::remove_cvref_t<EW>>::type;
    auto it = m_event_manager2.find(std::type_index(typeid(T)));
    FE_ASSERT(it != m_event_manager2.end() && "Event type not registered!");
    return std::remove_cvref_t<EW>(*it->second.get<T>());
  }

  template <IsResourceParam R>
  auto get_param(uint32_t passId) {
    using RawT = std::remove_cvref_t<R>;
    using U = typename RawT::type;

    auto it = m_resource_manager.find(std::type_index(typeid(U)));
    FE_ASSERT(it != m_resource_manager.end() && "Resource not registered!");

    return RawT(it->second);
  }

  void next_frame() {
    for (auto& [tid, swap] : m_event_swap) {
      swap(*this);
    }
    for (auto& [passId, ecb] : m_entity_command_buffers) {
      for (auto& [handle, entity] : ecb.m_entity_map) {
        if (entity == entt::null) {
          entity = m_registry.create();
        }
      }
      for (auto& e : ecb.m_destroyed_entities) {
        m_registry.destroy(e);
      }
    }
  }

 protected:
  Registry m_registry;

  stl::unordered_map<std::type_index, ResourceStorage> m_resource_manager;

  stl::unordered_map<std::type_index, ResourceStorage> m_event_manager1;
  stl::unordered_map<std::type_index, ResourceStorage> m_event_manager2;

  stl::unordered_map<std::type_index, void (*)(WorldBase&)> m_event_swap;

  stl::unordered_map<uint32_t, EntityCommandBuffer> m_entity_command_buffers;

  friend class Detail;
  friend class Pass;
};
}  // namespace fe::engine::ecs