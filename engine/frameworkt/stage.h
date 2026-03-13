#pragma once

#include "common.h"
#include <tuple>

namespace fe::engine::stage {

struct IsStage {};
struct None {};

// 确保类型包装为 tuple
template <typename T> struct ensure_tuple { using type = std::tuple<T>; };
template <typename... Ts> struct ensure_tuple<std::tuple<Ts...>> { using type = std::tuple<Ts...>; };
template <> struct ensure_tuple<None> { using type = std::tuple<>; };
template <typename T> using ensure_tuple_t = typename ensure_tuple<T>::type;

// 阶段基类：包含前置/后置阶段列表，以及是否重复执行
template <typename Prev = None, typename Next = None, typename Repeat = std::true_type>
class Stage : public IsStage {
public:
    using is_repeat = Repeat;
    using previous_list = ensure_tuple_t<Prev>;
    using next_list = ensure_tuple_t<Next>;
};

// 辅助别名
template <typename... Targets>
using after = Stage<std::tuple<Targets...>, None, std::conjunction<typename Targets::is_repeat...>>;

template <typename... Targets>
using before = Stage<None, std::tuple<Targets...>, std::conjunction<typename Targets::is_repeat...>>;

// 内置阶段
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

// 阶段哈希类型
using StageHash = size_t;

// 全局阶段信息注册表
inline stl::unordered_map<StageHash, stl::string> g_stage_names;
inline stl::unordered_map<StageHash, stl::vector<StageHash>> g_stage_before;
inline stl::unordered_map<StageHash, stl::vector<StageHash>> g_stage_after;

namespace detail {
    template <typename Tuple, size_t... Is>
    void fill_hashes(stl::vector<StageHash>& vec, std::index_sequence<Is...>) {
        (vec.push_back(get_stage_hash<std::tuple_element_t<Is, Tuple>>()), ...);
    }
} // namespace detail

template <typename T>
stl::vector<StageHash> get_previous_hashes() {
    using PList = typename T::previous_list;
    stl::vector<StageHash> result;
    result.reserve(std::tuple_size_v<PList>);
    detail::fill_hashes<PList>(result, std::make_index_sequence<std::tuple_size_v<PList>>{});
    return result;
}

template <typename T>
stl::vector<StageHash> get_next_hashes() {
    using NList = typename T::next_list;
    stl::vector<StageHash> result;
    result.reserve(std::tuple_size_v<NList>);
    detail::fill_hashes<NList>(result, std::make_index_sequence<std::tuple_size_v<NList>>{});
    return result;
}

template <typename T>
void init_stage_info() {
    static bool done = false;
    if (done) return;
    done = true;

    auto hash = typeid(T).hash_code();
    g_stage_names[hash] = typeid(T).name();

    auto befores = get_previous_hashes<T>();
    auto afters = get_next_hashes<T>();

    for (auto prev : befores) {
        g_stage_before[hash].push_back(prev);
        g_stage_after[prev].push_back(hash);
    }
    for (auto next : afters) {
        g_stage_after[hash].push_back(next);
        g_stage_before[next].push_back(hash);
    }
}

template <typename T>
StageHash get_stage_hash() {
    static_assert(std::is_base_of_v<IsStage, T>, "T must be a Stage");
    init_stage_info<T>();
    return typeid(T).hash_code();
}

} // namespace fe::engine::stage
