#pragma once

#include <type_traits>
#include <tuple>

namespace fe::engine::ecs::meta {
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

}  // namespace fe::engine::ecs::meta