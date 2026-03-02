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
concept IsComponentReader = is_component_reader<std::remove_cvref_t<T>>::value;
template <typename T>
concept IsComponentWriter = is_component_writer<std::remove_cvref_t<T>>::value;

template <typename T>
concept IsEventReader = is_event_reader<std::remove_cvref_t<T>>::value;
template <typename T>
concept IsEventWriter = is_event_writer<std::remove_cvref_t<T>>::value;

template <typename T>
concept IsResourceParam = !IsEntityQuery<T> && !IsEntityCreator<T> && !IsEntityDestroyer<T> && !IsComponentReader<T> && !IsComponentWriter<T> &&
                          !IsEventReader<T> && !IsEventWriter<T>;

class WorldBase {
 public:
  WorldBase() = default;
  virtual ~WorldBase() = default;

 protected:
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

  template <IsEventReader ER>
  auto get_param() {
    using T = typename is_event_reader<std::remove_cvref_t<ER>>::type;
    auto it = m_event_manager1.find(std::type_index(typeid(T)));
    FE_ASSERT(it != m_event_manager1.end() && "Event type not registered!");
    return std::remove_cvref_t<ER>(it->second);
  }
  template <IsEventWriter EW>
  auto get_param() {
    using T = typename is_event_writer<std::remove_cvref_t<EW>>::type;
    auto it = m_event_manager2.find(std::type_index(typeid(T)));
    FE_ASSERT(it != m_event_manager2.end() && "Event type not registered!");
    return std::remove_cvref_t<EW>(it->second);
  }

  template <IsResourceParam R>
  decltype(auto) get_param() {
    using RawT = std::remove_cvref_t<R>;

    auto it = m_resource_manager.find(std::type_index(typeid(RawT)));
    FE_ASSERT(it != m_resource_manager.end() && "Resource not registered!");

    return it->second.get<RawT>();
  }

 protected:
  Registry m_registry;

  stl::unordered_map<std::type_index, Resource> m_resource_manager;

  stl::unordered_map<std::type_index, Resource> m_event_manager1;
  stl::unordered_map<std::type_index, Resource> m_event_manager2;

  friend class Detail;
  friend class Pass;
};
}  // namespace fe::engine::ecs