#pragma once

#include <type_traits>
#include <tuple>

namespace fe::engine::ecs::stage {
struct IsStage {};
struct None {};

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

template <typename P = None, typename N = None, typename Repeat = std::true_type>
class Stage : public IsStage {
 public:
  using is_repeat = Repeat;
  using previous_list = typename ensure_tuple<P>::type;
  using next_list = typename ensure_tuple<N>::type;
};

template <typename... Targets>
using after = Stage<std::tuple<Targets...>, None, std::conjunction<typename Targets::is_repeat...>>;

template <typename... Targets>
using before = Stage<None, std::tuple<Targets...>, std::conjunction<typename Targets::is_repeat...>>;

class StartRoot : public Stage<None, None, std::false_type> {};
class EngineInit : public before<StartRoot> {};

class PreStartup : public after<EngineInit> {};
class Startup : public after<PreStartup> {};
class PostStartup : public after<Startup> {};

class First : public Stage<None, None, std::true_type> {};
class PreUpdate : public after<First> {};
class Update : public after<PreUpdate> {};
class PostUpdate : public after<Update> {};

class Last : public after<PostUpdate> {};
class Cleanup : public after<Last> {};
}  // namespace fe::engine::ecs::stage

#include <typeindex>
#include <vector>
#include <algorithm>

namespace fe::engine::ecs {
using StageHash = size_t;

template <typename T>
[[nodiscard]] constexpr StageHash get_stage_hash() noexcept {
  static_assert(std::is_base_of_v<IsStage, T>, "T must be a Stage");
  return typeid(T).hash_code();
}

namespace detail {
template <typename Tuple, size_t... Is>
void fill_hashes(std::vector<StageHash>& vec, std::index_sequence<Is...>) {
  (vec.push_back(get_stage_hash<std::tuple_element_t<Is, Tuple>>()), ...);
}
}  // namespace detail

template <typename T>
[[nodiscard]] std::vector<StageHash> get_previous_hashes() {
  using PList = typename T::previous_list;
  std::vector<StageHash> result;
  result.reserve(std::tuple_size_v<PList>);
  detail::fill_hashes<PList>(result, std::make_index_sequence<std::tuple_size_v<PList>>{});
  return result;
}

template <typename T>
[[nodiscard]] std::vector<StageHash> get_next_hashes() {
  using NList = typename T::next_list;
  std::vector<StageHash> result;
  result.reserve(std::tuple_size_v<NList>);
  detail::fill_hashes<NList>(result, std::make_index_sequence<std::tuple_size_v<NList>>{});
  return result;
}

}  // namespace fe::engine::ecs
