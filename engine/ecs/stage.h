#pragma once

#include <type_traits>
#include <tuple>
#include <typeindex>
#include <vector>
#include <algorithm>

#include "engine/base/pch.h"

namespace fe::engine::ecs::stage {
struct IsStage {};
struct None {};

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

template <>
struct ensure_tuple<None> {
  using type = std::tuple<>;
};

template <typename T>
using ensure_tuple_t = typename ensure_tuple<T>::type;

// ============================================================================
// Stage definitions
// ============================================================================

template <typename P = None, typename N = None, typename Repeat = std::true_type>
class Stage : public IsStage {
 public:
  using is_repeat = Repeat;
  using previous_list = ensure_tuple_t<P>;
  using next_list = ensure_tuple_t<N>;
};

template <typename... Targets>
using after = Stage<std::tuple<Targets...>, None, std::conjunction<typename Targets::is_repeat...>>;

template <typename... Targets>
using before = Stage<None, std::tuple<Targets...>, std::conjunction<typename Targets::is_repeat...>>;

// Built-in Stage definitions
class Init : public Stage<None, None, std::false_type> {};
class PreStartup : public after<Init> {};
class Startup : public after<PreStartup> {};
class PostStartup : public after<Startup> {};

class First : public Stage<None, None, std::true_type> {};
class PreUpdate : public after<First> {};
class Update : public after<PreUpdate> {};
class PostUpdate : public after<Update> {};
class Last : public after<PostUpdate> {};
class Cleanup : public after<Last> {};

// ============================================================================
// Stage Hash utilities
// ============================================================================

using StageHash = size_t;
inline stl::unordered_map<size_t, stl::string> g_stage_name_registry;

inline stl::unordered_map<StageHash, stl::vector<StageHash>> g_stage_before_map;
inline stl::unordered_map<StageHash, stl::vector<StageHash>> g_stage_after_map;

template <typename T>
void init_stage_info() {
  static bool initialized = false;
  if (initialized)
    return;

  initialized = true;

  auto stage_hash = typeid(T).hash_code();
  g_stage_name_registry.insert({stage_hash, std::type_index(typeid(T)).name()});

  auto befores = get_previous_hashes<T>();
  auto afters = get_next_hashes<T>();

  for (auto prev_hash : befores) {
    auto& my_befores = g_stage_before_map[stage_hash];
    if (std::find(my_befores.begin(), my_befores.end(), prev_hash) == my_befores.end()) {
      my_befores.push_back(prev_hash);
    }

    auto& prev_afters = g_stage_after_map[prev_hash];
    if (std::find(prev_afters.begin(), prev_afters.end(), stage_hash) == prev_afters.end()) {
      prev_afters.push_back(stage_hash);
    }
  }

  for (auto next_hash : afters) {
    auto& my_afters = g_stage_after_map[stage_hash];
    if (std::find(my_afters.begin(), my_afters.end(), next_hash) == my_afters.end()) {
      my_afters.push_back(next_hash);
    }

    auto& next_befores = g_stage_before_map[next_hash];
    if (std::find(next_befores.begin(), next_befores.end(), stage_hash) == next_befores.end()) {
      next_befores.push_back(stage_hash);
    }
  }
}

// Note: typeid(T).hash_code() is not constexpr in standard C++
template <typename T>
[[nodiscard]] StageHash get_stage_hash() noexcept {
  static_assert(std::is_base_of_v<IsStage, T>, "T must be a Stage");
  init_stage_info<T>();
  return typeid(T).hash_code();
}

namespace detail {
template <typename Tuple, size_t... Is>
void fill_hashes(stl::vector<StageHash>& vec, std::index_sequence<Is...>) {
  (vec.push_back(get_stage_hash<std::tuple_element_t<Is, Tuple>>()), ...);
}
}  // namespace detail

template <typename T>
[[nodiscard]] stl::vector<StageHash> get_previous_hashes() {
  using PList = typename T::previous_list;
  stl::vector<StageHash> result;
  result.reserve(std::tuple_size_v<PList>);
  detail::fill_hashes<PList>(result, std::make_index_sequence<std::tuple_size_v<PList>>{});
  return result;
}

template <typename T>
[[nodiscard]] stl::vector<StageHash> get_next_hashes() {
  using NList = typename T::next_list;
  stl::vector<StageHash> result;
  result.reserve(std::tuple_size_v<NList>);
  detail::fill_hashes<NList>(result, std::make_index_sequence<std::tuple_size_v<NList>>{});
  return result;
}

}  // namespace fe::engine::ecs::stage
