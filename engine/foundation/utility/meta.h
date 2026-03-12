#pragma once

#include <type_traits>
#include <tuple>

namespace fe::engine::meta {
template <typename T, typename... Us>
inline constexpr bool is_in_pack = (std::is_same_v<T, Us> || ...);

// Basic type cleanup
template <typename T>
using clean_t = std::remove_cv_t<std::remove_reference_t<T>>;

// ============================================================================
// function_traits: extracts parameter types from callable objects
// ============================================================================

template <typename T>
struct function_traits : function_traits<decltype(&T::operator())> {};

template <typename R, typename... Args>
struct function_traits<R (*)(Args...)> {
  using args_tuple = std::tuple<Args...>;
  using return_type = R;
  static constexpr size_t arity = sizeof...(Args);
};

template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const> {
  using args_tuple = std::tuple<Args...>;
  using return_type = R;
  static constexpr size_t arity = sizeof...(Args);
};

template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...)> {
  using args_tuple = std::tuple<Args...>;
  using return_type = R;
  static constexpr size_t arity = sizeof...(Args);
};

// ============================================================================
// ensure_tuple: ensures type is wrapped as a tuple
// ============================================================================

template <typename T>
struct ensure_tuple {
  using type = std::tuple<T>;
};

template <typename... Ts>
struct ensure_tuple<std::tuple<Ts...>> {
  using type = std::tuple<Ts...>;
};

template <typename T>
using ensure_tuple_t = typename ensure_tuple<T>::type;

// ============================================================================
// TypeList: type list container
// ============================================================================

template <typename... T>
struct TypeList {};

// ============================================================================
// Type classification helpers
// ============================================================================

// Checks if type is an lvalue reference return (used for parameter capture decisions)
template <typename T>
struct is_lvalue_reference_return : std::false_type {};

template <typename T>
inline constexpr bool is_lvalue_reference_return_v = is_lvalue_reference_return<T>::value;

// Helper constant that always returns true
template <typename>
inline constexpr bool always_false_v = false;

// ============================================================================
// Component pack subset check
// ============================================================================

// Check if T is in pack<Components...>
template <typename T, typename... Components>
constexpr bool is_in_pack_v = (std::is_same_v<T, Components> || ...);

// Check if all TargetTypes are in SourceTypes pack
template <typename TargetPack, typename SourcePack>
struct is_subset_of_impl;

template <typename... Targets, typename... Sources>
struct is_subset_of_impl<std::tuple<Targets...>, std::tuple<Sources...>> : std::bool_constant<(is_in_pack_v<Targets, Sources...> && ...)> {};

template <typename TargetPack, typename SourcePack>
constexpr bool is_subset_of_v = is_subset_of_impl<TargetPack, SourcePack>::value;

// ============================================================================
// Type hash utilities (compile-time friendly)
// ============================================================================

template <typename T>
inline constexpr size_t type_hash_v = typeid(T).hash_code();

// Get hash for type, handling const/volatile/reference
template <typename T>
inline constexpr size_t clean_type_hash_v = type_hash_v<clean_t<T>>;

// ============================================================================
// Template for generating type hash arrays at compile time
// ============================================================================

template <typename... Ts>
constexpr stl::array<size_t, sizeof...(Ts)> make_type_hashes() {
  return {type_hash_v<Ts>...};
}

// ============================================================================
// Remove first type from tuple
// ============================================================================

template <typename Tuple>
struct remove_first;

template <typename T, typename... Ts>
struct remove_first<std::tuple<T, Ts...>> {
  using type = std::tuple<Ts...>;
};

template <typename Tuple>
using remove_first_t = typename remove_first<Tuple>::type;

// ============================================================================
// Check if type has a specific nested type
// ============================================================================

template <typename T, typename = void>
struct has_access_info : std::false_type {};

template <typename T>
struct has_access_info<T, std::void_t<typename T::AccessInfo>> : std::true_type {};

template <typename T>
inline constexpr bool has_access_info_v = has_access_info<T>::value;

}  // namespace fe::engine::meta