#pragma once

#include "common.h"

#include "access/param.h"
#include "resource/resourceManager.h"

namespace fe::engine::ecs {
template <typename T>
concept IsEntityQuery = is_entity_query<std::remove_cvref_t<T>>::value;
template <typename T>
concept IsEntityCreator = is_entity_creator<std::remove_cvref_t<T>>::value;
template <typename T>
concept IsEntityDestroyer = is_entity_destroyer<std::remove_cvref_t<T>>::value;

template <typename T>
concept IsComponentReader = is_component_reader<std::remove_cvref_t<T>>::value;
template <typename T>
concept IsComponentWriter = is_component_writer<std::remove_cvref_t<T>>::value;

template <typename T>
concept IsResourceReader = is_resource_reader<std::remove_cvref_t<T>>::value;
template <typename T>
concept IsResourceWriter = is_resource_writer<std::remove_cvref_t<T>>::value;

template <typename T>
concept IsEventReader = is_event_reader<std::remove_cvref_t<T>>::value;
template <typename T>
concept IsEventWriter = is_event_writer<std::remove_cvref_t<T>>::value;

class WorldBase {
 public:
  WorldBase() = default;
  virtual ~WorldBase() = default;

  template <IsEntityQuery EQ>
  auto get_param() {
    return std::remove_cvref_t<EQ>(m_registry);
  }
  template <IsEntityCreator EC>
  auto get_param() {
    return std::remove_cvref_t<EC>(m_registry);
  }
  template <IsEntityDestroyer ED>
  auto get_param() {
    return std::remove_cvref_t<ED>(m_registry);
  }

  template <IsComponentReader CR>
  auto get_param() {
    return std::remove_cvref_t<CR>(m_registry);
  }
  template <IsComponentWriter CW>
  auto get_param() {
    return std::remove_cvref_t<CW>(m_registry);
  }

  template <IsResourceReader RR>
  auto get_param() {
    return std::remove_cvref_t<RR>(m_registry);
  }
  template <IsResourceWriter RW>
  auto get_param() {
    return std::remove_cvref_t<RW>(m_registry);
  }

  template <IsEventReader ER>
  auto get_param() {
    return std::remove_cvref_t<ER>(m_registry);
  }
  template <IsEventWriter EW>
  auto get_param() {
    return std::remove_cvref_t<EW>(m_registry);
  }

 protected:
  Registry m_registry;
  ResourceManager m_resource_manager;

  ResourceManager m_event_reader_manager;
  ResourceManager m_event_writer_manager;
};
}  // namespace fe::engine::ecs