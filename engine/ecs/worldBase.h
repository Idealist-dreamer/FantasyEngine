#pragma once

#include "common.h"
#include "context.h"

#include "accessEntity.h"
#include "accessComponent.h"
#include "accessEvent.h"
#include "accessContext.h"

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
concept IsContextParam = is_resource_reader<T>::value || is_resource_writer<T>::value;

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
    m_registry.storage<T>();
    m_registry.storage<AddComponentTag<T>>();
    m_registry.storage<ChangeComponentTag<T>>();
    m_registry.storage<RemoveComponentTag<T>>();
    m_registry.storage<AddComponentDelayed<T>>();
    m_registry.storage<ChangeComponentDelayed<T>>();
    m_registry.storage<RemoveComponentDelayed<T>>();
  }

  template <typename T, typename... Args>
  void register_context(Args&&... args) {
    auto tid = std::type_index(typeid(T));
    if (m_resource_manager.find(tid) == m_resource_manager.end()) {
      m_resource_manager.insert({tid, ContextStorage::create<T>(std::forward<Args>(args)...)});
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
        auto& data1 = *(wb.m_event_manager1[inner_tid].get<stl::vector<T>>());
        auto& data2 = *(wb.m_event_manager2[inner_tid].get<stl::vector<T>>());

        data1.clear();
        data1.swap(data2);
      };
    }
  }

  template <typename R, typename First, typename... Args>
  static constexpr First get_first_arg(R (*)(First, Args...));

  template <auto Candidate>
  static constexpr void check_signature() {
    // 直接推导第一个参数类型
    using FirstArg = decltype(get_first_arg(Candidate));

    // 验证是否为 const Registry&
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
  decltype(auto) get_param(uint32_t passId) {
    return (m_entity_command_buffers[passId]);
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

  template <IsContextParam R>
  auto get_param(uint32_t passId) {
    using RawT = std::remove_cvref_t<R>;
    using U = typename RawT::type;

    auto it = m_resource_manager.find(std::type_index(typeid(U)));
    FE_ASSERT(it != m_resource_manager.end() && "Context not registered!");

    return RawT(it->second);
  }

 protected:
  Registry m_registry;

  stl::unordered_map<std::type_index, ContextStorage> m_resource_manager;

  stl::unordered_map<std::type_index, ContextStorage> m_event_manager1;
  stl::unordered_map<std::type_index, ContextStorage> m_event_manager2;

  stl::unordered_map<std::type_index, void (*)(WorldBase&)> m_event_swap;

  stl::unordered_map<uint32_t, EntityCommandBuffer> m_entity_command_buffers;

  friend class Detail;
  friend class Pass;
};
}  // namespace fe::engine::ecs