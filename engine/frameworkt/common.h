#pragma once

#include <entt/entt.hpp>
#include <functional>
#include <typeindex>
#include <type_traits>
#include <tuple>
#include <cstdint>

#include "foundation/container/stl.h"
#include "foundation/utility/any.h"

namespace fe::engine {

// 实体与注册表别名
using Entity = entt::entity;
using Registry = entt::registry;

// 类型清理工具
template <typename T>
using clean_t = std::remove_cv_t<std::remove_reference_t<T>>;

// 优先级常量
enum Priority : uint32_t { First = 0, High = 1000, Mid = 2000, Low = 3000 };

// 组件延迟操作标签（用于双缓冲、变更跟踪）
template <typename T> struct AddTag {};
template <typename T> struct ChangeTag {};
template <typename T> struct RemoveTag {};
template <typename T> struct AddDelayed { T data; };
template <typename T> struct ChangeDelayed { T data; };
template <typename T> struct RemoveDelayed {};

// 基础类型萃取
template <typename T> struct base_type { using type = T; };
template <typename T> struct base_type<AddTag<T>> { using type = T; };
template <typename T> struct base_type<ChangeTag<T>> { using type = T; };
template <typename T> struct base_type<RemoveTag<T>> { using type = T; };
template <typename T> struct base_type<AddDelayed<T>> { using type = T; };
template <typename T> struct base_type<ChangeDelayed<T>> { using type = T; };
template <typename T> struct base_type<RemoveDelayed<T>> { using type = T; };

template <typename T>
using base_type_t = typename base_type<T>::type;

// 组件兼容性检查
template <typename Req, typename Decl>
struct is_compatible : std::is_same<std::remove_const_t<base_type_t<Req>>,
                                    std::remove_const_t<base_type_t<Decl>>> {};

} // namespace fe::engine
