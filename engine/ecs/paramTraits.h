#pragma once

#include <type_traits>

#include "paramTypes.h"
#include "meta.h"

namespace fe::engine::ecs {

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
// Context (Resource) Type Traits
// ============================================================================

template <typename T>
struct is_resource_reader : std::false_type {};

template <typename T>
struct is_resource_reader<ContextReader<T>> : std::true_type {
  using type = T;
};

template <typename T>
struct is_resource_writer : std::false_type {};

template <typename T>
struct is_resource_writer<ContextWriter<T>> : std::true_type {
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
concept IsContextParam = is_resource_reader<meta::clean_t<T>>::value || is_resource_writer<meta::clean_t<T>>::value;

// ============================================================================
// Parameter passing mode markers
// EntityCommandBuffer requires pass-by-reference, others pass-by-value
// ============================================================================

template <typename T>
struct param_pass_by_value : std::bool_constant<!is_entity_command_buffer<meta::clean_t<T>>::value> {};

template <typename T>
inline constexpr bool param_pass_by_value_v = param_pass_by_value<T>::value;

}  // namespace fe::engine::ecs
