#pragma once

#include "paramMutex.h"
#include "paramTypes.h"

#include "sceneBase.h"

namespace fe::engine {

// =============================================================================
// ParamAdapter: Unified parameter handling protocol
// Each parameter type declares its own mutex, preparation, and fetch logic.
// =============================================================================

template <typename T>
struct ParamAdapter {
  using CleanT = meta::clean_t<T>;

  static stl::vector<Mutex> get_mutexes() { return {}; }

  static void prepare(SceneBase&, uint32_t) {}

  static auto fetch(SceneBase& scene, uint32_t) { return CleanT(scene.m_registry); }
};

// =============================================================================
// Entity types
// =============================================================================

template <>
struct ParamAdapter<EntityQuery> {
  static stl::vector<Mutex> get_mutexes() { return {Mutex::query_entity()}; }
  static void prepare(SceneBase&, uint32_t) {}
  static EntityQuery fetch(SceneBase& scene, uint32_t) { return EntityQuery(scene.m_registry); }
};

template <>
struct ParamAdapter<EntityCreator> {
  static stl::vector<Mutex> get_mutexes() { return {Mutex::create_entity()}; }
  static void prepare(SceneBase&, uint32_t) {}
  static EntityCreator fetch(SceneBase& scene, uint32_t) { return EntityCreator(scene.m_registry); }
};

template <>
struct ParamAdapter<EntityDestroyer> {
  static stl::vector<Mutex> get_mutexes() { return {Mutex::destroy_entity()}; }
  static void prepare(SceneBase&, uint32_t) {}
  static EntityDestroyer fetch(SceneBase& scene, uint32_t) { return EntityDestroyer(scene.m_registry); }
};

template <>
struct ParamAdapter<EntityCommandBuffer> {
  static stl::vector<Mutex> get_mutexes() { return {}; }
  static void prepare(SceneBase& scene, uint32_t passId) {
    if (scene.m_entity_command_buffers.find(passId) == scene.m_entity_command_buffers.end()) {
      scene.m_entity_command_buffers.emplace(passId, EntityCommandBuffer{});
    }
  }
  static decltype(auto) fetch(SceneBase& scene, uint32_t passId) { return std::ref(scene.m_entity_command_buffers[passId]); }
};

// =============================================================================
// Component types
// =============================================================================

template <typename... Cs>
struct ParamAdapter<ComponentReader<Cs...>> {
  static stl::vector<Mutex> get_mutexes() { return {Mutex::read_component<Cs>()...}; }
  static void prepare(SceneBase& scene, uint32_t) {
    (scene.m_registry.template storage<std::remove_const_t<Cs>>(), ...);
    (scene.m_registry.template storage<AddComponentTag<Cs>>(), ...);
    (scene.m_registry.template storage<ChangeComponentTag<Cs>>(), ...);
    (scene.m_registry.template storage<RemoveComponentTag<Cs>>(), ...);
    (scene.m_registry.template storage<AddComponentDelayed<Cs>>(), ...);
    (scene.m_registry.template storage<ChangeComponentDelayed<Cs>>(), ...);
    (scene.m_registry.template storage<RemoveComponentDelayed<Cs>>(), ...);
  }
  static ComponentReader<Cs...> fetch(SceneBase& scene, uint32_t) { return ComponentReader<Cs...>(scene.m_registry); }
};

template <typename... Cs>
struct ParamAdapter<ComponentWriter<Cs...>> {
  static stl::vector<Mutex> get_mutexes() { return {Mutex::write_component<Cs>()...}; }
  static void prepare(SceneBase& scene, uint32_t) {
    (scene.m_registry.template storage<std::remove_const_t<Cs>>(), ...);
    (scene.m_registry.template storage<AddComponentTag<Cs>>(), ...);
    (scene.m_registry.template storage<ChangeComponentTag<Cs>>(), ...);
    (scene.m_registry.template storage<RemoveComponentTag<Cs>>(), ...);
    (scene.m_registry.template storage<AddComponentDelayed<Cs>>(), ...);
    (scene.m_registry.template storage<ChangeComponentDelayed<Cs>>(), ...);
    (scene.m_registry.template storage<RemoveComponentDelayed<Cs>>(), ...);
  }
  static ComponentWriter<Cs...> fetch(SceneBase& scene, uint32_t) { return ComponentWriter<Cs...>(scene.m_registry); }
};

// =============================================================================
// Event types
// =============================================================================

template <typename T>
struct ParamAdapter<EventReader<T>> {
  static stl::vector<Mutex> get_mutexes() { return {Mutex::read_event<T>()}; }
  static void prepare(SceneBase& scene, uint32_t) {
    auto tid = std::type_index(typeid(T));
    if (scene.m_event_manager1.find(tid) == scene.m_event_manager1.end()) {
      scene.m_event_manager1.emplace(tid, Any::create<stl::vector<T>>());
      scene.m_event_manager2.emplace(tid, Any::create<stl::vector<T>>());
      scene.m_event_swap[tid] = [](SceneBase& s) {
        auto it = std::type_index(typeid(T));
        auto& d1 = *s.m_event_manager1[it].template get<stl::vector<T>>();
        auto& d2 = *s.m_event_manager2[it].template get<stl::vector<T>>();
        d1.swap(d2);
        d2.clear();
      };
    }
  }
  static EventReader<T> fetch(SceneBase& scene, uint32_t) {
    return EventReader<T>(*scene.m_event_manager1[std::type_index(typeid(T))].template get<stl::vector<T>>());
  }
};

template <typename T>
struct ParamAdapter<EventWriter<T>> {
  static stl::vector<Mutex> get_mutexes() { return {Mutex::write_event<T>()}; }
  static void prepare(SceneBase& scene, uint32_t) {
    auto tid = std::type_index(typeid(T));
    if (scene.m_event_manager1.find(tid) == scene.m_event_manager1.end()) {
      scene.m_event_manager1.emplace(tid, Any::create<stl::vector<T>>());
      scene.m_event_manager2.emplace(tid, Any::create<stl::vector<T>>());
      scene.m_event_swap[tid] = [](SceneBase& s) {
        auto it = std::type_index(typeid(T));
        auto& d1 = *s.m_event_manager1[it].template get<stl::vector<T>>();
        auto& d2 = *s.m_event_manager2[it].template get<stl::vector<T>>();
        d1.swap(d2);
        d2.clear();
      };
    }
  }
  static EventWriter<T> fetch(SceneBase& scene, uint32_t) {
    return EventWriter<T>(*scene.m_event_manager2[std::type_index(typeid(T))].template get<stl::vector<T>>());
  }
};

// =============================================================================
// Context types
// =============================================================================

template <typename T>
struct ParamAdapter<ContextReader<T>> {
  static stl::vector<Mutex> get_mutexes() { return {Mutex::read_context<T>()}; }
  static void prepare(SceneBase& scene, uint32_t) {
    auto tid = std::type_index(typeid(T));
    if (scene.m_context_manager.find(tid) == scene.m_context_manager.end()) {
      scene.m_context_manager.emplace(tid, Any{});
    }
  }
  static Context<T> fetch(SceneBase& scene, uint32_t) { return Context<T>(scene.m_context_manager[std::type_index(typeid(T))]); }
};

template <typename T>
struct ParamAdapter<ContextWriter<T>> {
  static stl::vector<Mutex> get_mutexes() { return {Mutex::write_context<T>()}; }
  static void prepare(SceneBase& scene, uint32_t) {
    auto tid = std::type_index(typeid(T));
    if (scene.m_context_manager.find(tid) == scene.m_context_manager.end()) {
      scene.m_context_manager.emplace(tid, Any{});
    }
  }
  static Context<T> fetch(SceneBase& scene, uint32_t) { return Context<T>(scene.m_context_manager[std::type_index(typeid(T))]); }
};

// =============================================================================
// Helper: Aggregate operations for parameter packs
// =============================================================================

struct ParamOps {
  template <typename... Args>
  static stl::vector<Mutex> collect_mutexes() {
    stl::vector<Mutex> result;
    size_t total = (ParamAdapter<meta::clean_t<Args>>::get_mutexes().size() + ... + 0);
    result.reserve(total);
    (append_mutexes<meta::clean_t<Args>>(result), ...);
    return result;
  }

  template <typename... Args>
  static stl::vector<std::function<void(SceneBase&)>> collect_preparers(uint32_t passId) {
    stl::vector<std::function<void(SceneBase&)>> result;
    (result.push_back([passId](SceneBase& s) { ParamAdapter<meta::clean_t<Args>>::prepare(s, passId); }), ...);
    return result;
  }

  template <typename... Args>
  static auto fetch_all(SceneBase& scene, uint32_t passId) {
    return std::make_tuple(ParamAdapter<meta::clean_t<Args>>::fetch(scene, passId)...);
  }

 private:
  template <typename T>
  static void append_mutexes(stl::vector<Mutex>& out) {
    auto m = ParamAdapter<T>::get_mutexes();
    out.insert(out.end(), m.begin(), m.end());
  }
};

// ============================================================================
// Entity Type Traits
// ============================================================================

template <typename T>
struct is_entity_query : std::false_type {};

template <>
struct is_entity_query<EntityQuery> : std::true_type {};

template <typename T>
struct is_entity_creator : std::false_type {};

template <>
struct is_entity_creator<EntityCreator> : std::true_type {};

template <typename T>
struct is_entity_destroyer : std::false_type {};

template <>
struct is_entity_destroyer<EntityDestroyer> : std::true_type {};

template <typename T>
struct is_entity_command_buffer : std::false_type {};

template <>
struct is_entity_command_buffer<EntityCommandBuffer> : std::true_type {};

// ============================================================================
// Component Type Traits
// ============================================================================

template <typename T>
struct is_component_reader : std::false_type {};

template <typename... Components>
struct is_component_reader<ComponentReader<Components...>> : std::true_type {
  using component_types = std::tuple<Components...>;
};

template <typename T>
struct is_component_writer : std::false_type {};

template <typename... Components>
struct is_component_writer<ComponentWriter<Components...>> : std::true_type {
  using component_types = std::tuple<Components...>;
};

// ============================================================================
// Event Type Traits
// ============================================================================

template <typename T>
struct is_event_reader : std::false_type {};

template <typename T>
struct is_event_reader<EventReader<T>> : std::true_type {
  using type = stl::vector<T>;
};

template <typename T>
struct is_event_writer : std::false_type {};

template <typename T>
struct is_event_writer<EventWriter<T>> : std::true_type {
  using type = stl::vector<T>;
};

// ============================================================================
// Context (context) Type Traits
// ============================================================================

template <typename T>
struct is_context_reader : std::false_type {};

template <typename T>
struct is_context_reader<ContextReader<T>> : std::true_type {
  using type = T;
};

template <typename T>
struct is_context_writer : std::false_type {};

template <typename T>
struct is_context_writer<ContextWriter<T>> : std::true_type {
  using type = T;
};

// ============================================================================
// Concepts
// ============================================================================

template <typename T>
concept IsEntityQuery = is_entity_query<meta::clean_t<T>>::value;

template <typename T>
concept IsEntityCreator = is_entity_creator<meta::clean_t<T>>::value;

template <typename T>
concept IsEntityDestroyer = is_entity_destroyer<meta::clean_t<T>>::value;

template <typename T>
concept IsEntityCommandBuffer = is_entity_command_buffer<meta::clean_t<T>>::value;

template <typename T>
concept IsComponentReader = is_component_reader<meta::clean_t<T>>::value;

template <typename T>
concept IsComponentWriter = is_component_writer<meta::clean_t<T>>::value;

template <typename T>
concept IsEventReader = is_event_reader<meta::clean_t<T>>::value;

template <typename T>
concept IsEventWriter = is_event_writer<meta::clean_t<T>>::value;

template <typename T>
concept IsContextParam = is_context_reader<meta::clean_t<T>>::value || is_context_writer<meta::clean_t<T>>::value;

// ============================================================================
// Parameter passing mode markers
// EntityCommandBuffer requires pass-by-reference, others pass-by-value
// ============================================================================

template <typename T>
struct param_pass_by_value : std::bool_constant<!is_entity_command_buffer<meta::clean_t<T>>::value> {};

template <typename T>
inline constexpr bool param_pass_by_value_v = param_pass_by_value<T>::value;

}  // namespace fe::engine