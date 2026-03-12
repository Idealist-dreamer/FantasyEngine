#pragma once

#include <functional>
#include <string>

#include "context_adapter.h"

namespace fe::engine {

class Pass final {
  using CallType = std::function<void()>;
  static inline uint32_t s_next_id = 0;

 public:
  Pass(const stl::string& name) : m_id(s_next_id++), m_name(name) {}

  template <typename Func>
  void init(Func&& func) {
    using CleanFunc = std::remove_cvref_t<Func>;
    using ArgsTuple = typename meta::function_traits<CleanFunc>::args_tuple;
    init_impl<CleanFunc>(std::forward<Func>(func), static_cast<ArgsTuple*>(nullptr));
  }

  const uint32_t m_id;
  stl::string m_name;
  size_t m_stage = 0;
  ContextMutex m_mutex;

 private:
  template <typename Func, typename... Args>
  void init_impl(Func&& func, std::tuple<Args...>*) {
    // 1. 编译期静态计算出 Mutex
    m_mutex = ContextOps::collect_mutexes<Args...>();

    uint32_t pass_id = m_id;

    // 2. 准备阶段：确保在 Scene 实例化此 Pass 时注入必须依赖
    m_preparer = [pass_id](SceneContext& sc) {
      ContextOps::prepare_all<Args...>(sc, pass_id);
    };

    // 3. 绑定闭包：在 Scene::compile() 图构建时执行，提取裸指针/轻量代理结构并进行 Capture
    m_binder = [func = std::forward<Func>(func), pass_id](SceneContext& sc) -> CallType {
      return [func, bound_args = ContextOps::bind_all<Args...>(sc, pass_id)]() mutable {
        // 运行时完全没有查表、没有任何类型转换！直接在 CPU L1 Cache 愉快执行！
        std::apply(func, bound_args);
      };
    };
  }

  std::function<void(SceneContext&)> m_preparer;
  std::function<CallType(SceneContext&)> m_binder;
  CallType m_execute;

  friend class Scene;
};

}  // namespace fe::engine