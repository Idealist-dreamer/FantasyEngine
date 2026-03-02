#pragma once

#include "common.h"
#include "ResourceStorage.h"

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
    return std::remove_cvref_t<ER>(*it->second.get<T>());
  }
  template <IsEventWriter EW>
  auto get_param() {
    using T = typename is_event_writer<std::remove_cvref_t<EW>>::type;
    auto it = m_event_manager2.find(std::type_index(typeid(T)));
    FE_ASSERT(it != m_event_manager2.end() && "Event type not registered!");

    m_write_event_clear.push_back([this](WorldBase& w) {
      auto it = w.m_event_manager2.find(std::type_index(typeid(T)));
      auto& vector = *it->second.get<T>();
      vector.clear();
    });
    return std::remove_cvref_t<ER>(*it->second.get<T>());
  }

  template <IsResourceParam R>
  decltype(auto) get_param() {
    using RawT = std::remove_cvref_t<R>;

    auto it = m_resource_manager.find(std::type_index(typeid(RawT)));
    FE_ASSERT(it != m_resource_manager.end() && "ResourceStorage not registered!");

    return (*it->second.get<RawT>());
  }

  void next_frame() {
    std::swap(m_event_manager1, m_event_manager2);
    for (auto& clear_func : m_write_event_clear) {
      clear_func(*this);
    }
    m_write_event_clear.clear();
  }

 protected:
  Registry m_registry;

  stl::unordered_map<std::type_index, ResourceStorage> m_resource_manager;

  stl::unordered_map<std::type_index, ResourceStorage> m_event_manager1;
  stl::unordered_map<std::type_index, ResourceStorage> m_event_manager2;

  stl::vector<stl::function<void(WorldBase&)>> m_write_event_clear;

  friend class Detail;
  friend class Pass;
};
}  // namespace fe::engine::ecs