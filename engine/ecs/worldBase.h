#pragma once

#include "common.h"
#include "context.h"
#include "asset.h"

#include "paramTypes.h"
#include "paramTraits.h"
#include "paramAccess.h"

namespace fe::engine::ecs {

class WorldBase {
 public:
  WorldBase() = default;
  virtual ~WorldBase() = default;

  template <typename... Owned, typename... Get, typename... Exclude>
  void define_group(entt::get_t<Get...> = {}, entt::exclude_t<Exclude...> = {}) {
    m_registry.group<Owned...>(entt::get<Get...>, entt::exclude<Exclude...>);
  }

  template <typename T>
  void register_component() {
    m_registry.template storage<T>();
    m_registry.template storage<AddComponentTag<T>>();
    m_registry.template storage<ChangeComponentTag<T>>();
    m_registry.template storage<RemoveComponentTag<T>>();
    m_registry.template storage<AddComponentDelayed<T>>();
    m_registry.template storage<ChangeComponentDelayed<T>>();
    m_registry.template storage<RemoveComponentDelayed<T>>();
  }

  template <typename T, typename... Args>
  void register_context(Args&&... args) {
    auto tid = std::type_index(typeid(T));
    if (m_context_manager.find(tid) == m_context_manager.end()) {
      m_context_manager.insert({tid, ContextStorage::create<T>(std::forward<Args>(args)...)});
    }
  }

  template <typename T>
  void register_event() {
    auto tid = std::type_index(typeid(stl::vector<T>));

    if (m_event_manager1.find(tid) == m_event_manager1.end()) {
      m_event_manager1.insert({tid, ContextStorage::create<stl::vector<T>>()});
      m_event_manager2.insert({tid, ContextStorage::create<stl::vector<T>>()});

      m_event_swap[tid] = [](WorldBase& wb) {
        auto inner_tid = std::type_index(typeid(stl::vector<T>));
        auto& data1 = *(wb.m_event_manager1[inner_tid].template get<stl::vector<T>>());
        auto& data2 = *(wb.m_event_manager2[inner_tid].template get<stl::vector<T>>());

        data1.clear();
        data1.swap(data2);
      };
    }
  }

  // Member function pointer overloads for reactive callbacks
  template <typename R, typename First, typename... Args>
  static constexpr First get_first_arg(R (*)(First, Args...));

  template <typename R, typename C, typename First, typename... Args>
  static constexpr First get_first_arg(R (C::*)(First, Args...) const);

  template <typename R, typename C, typename First, typename... Args>
  static constexpr First get_first_arg(R (C::*)(First, Args...));

  template <auto Candidate>
  static constexpr void check_signature() {
    using FirstArg = decltype(get_first_arg(Candidate));
    static_assert(std::is_same_v<FirstArg, const Registry&>,
                  "Reactive callback MUST use 'const Registry&' as the first argument to ensure thread safety.");
  }

  template <auto Candidate, typename Type>
  void listen_entity_create(Type* instance) {
    check_signature<Candidate>();
    m_registry.on_construct<entt::entity>().template connect<Candidate>(instance);
  }

  template <auto Candidate, typename Type>
  void listen_entity_destroy(Type* instance) {
    check_signature<Candidate>();
    m_registry.on_destroy<entt::entity>().template connect<Candidate>(instance);
  }

  template <auto Candidate, typename Type>
  void listen_entity_update(Type* instance) {
    check_signature<Candidate>();
    m_registry.on_update<entt::entity>().template connect<Candidate>(instance);
  }

  template <typename Component, auto Candidate, typename Type>
  void listen_component_add(Type* instance) {
    check_signature<Candidate>();
    m_registry.on_construct<Component>().template connect<Candidate>(instance);
  }

  template <typename Component, auto Candidate, typename Type>
  void listen_component_remove(Type* instance) {
    check_signature<Candidate>();
    m_registry.on_destroy<Component>().template connect<Candidate>(instance);
  }

  template <typename Component, auto Candidate, typename Type>
  void listen_component_update(Type* instance) {
    check_signature<Candidate>();
    m_registry.on_update<Component>().template connect<Candidate>(instance);
  }

 protected:
  Registry m_registry;

  stl::unordered_map<std::type_index, ContextStorage> m_context_manager;

  stl::unordered_map<std::type_index, ContextStorage> m_event_manager1;
  stl::unordered_map<std::type_index, ContextStorage> m_event_manager2;

  stl::unordered_map<std::type_index, void (*)(WorldBase&)> m_event_swap;

  stl::unordered_map<uint32_t, EntityCommandBuffer> m_entity_command_buffers;

  friend class Pass;
  friend class World;
  friend struct PreparerCollector;
};

}  // namespace fe::engine::ecs
