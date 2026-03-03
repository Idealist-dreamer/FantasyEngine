#pragma once

#include "worldBase.h"

#include "paramMutex.h"
#include "paramDetail.h"

namespace fe::engine::ecs {
class WorldBase;

enum Priority : uint32_t { First = 0x00000000, High = 0x00001000, Mid = 0x00002000, Low = 0x00003000 };

class Pass {
 public:
  using CallType = std::function<void()>;

  Pass(const stl::string& name, bool isRepeat = true, uint32_t priority = uint32_t(Priority::Low))
      : m_name(name), m_repeat(isRepeat), m_priority(priority) {}
  ~Pass() = default;

  template <typename Func>
  static Pass create_start(const stl::string& name, Func&& func, uint32_t priority = uint32_t(Priority::Low)) {
    Pass pass(name, false, priority);
    pass.init(std::forward<Func>(func));
    return pass;
  }

  template <typename Func>
  static Pass create_update(const stl::string& name, Func&& func, uint32_t priority = uint32_t(Priority::Low)) {
    Pass pass(name, true, priority);
    pass.init(std::forward<Func>(func));
    return pass;
  }

  // template <typename Func>
  // void init(Func&& func) {
  //   using CleanFunc = std::remove_cvref_t<Func>;
  //   using ArgsTuple = typename function_traits<CleanFunc>::args_tuple;
  //   init_impl(std::forward<Func>(func), ArgsTuple{});
  // }

  template <typename Func>
  void init(Func&& func) {
    using CleanFunc = std::remove_cvref_t<Func>;
    // 仅提取类型包，不实例化元组
    using ArgsTuple = typename function_traits<CleanFunc>::args_tuple;

    // 调用新的 init_impl，通过模板参数传递 Args，而不是函数参数
    init_impl<CleanFunc>(std::forward<Func>(func), (ArgsTuple*)nullptr);
  }

  template <typename T>
  Pass& set_stage() {
    m_stage = typeid(T).name();
    return *this;
  }

  template <typename T>
  Pass& set_after_stage() {
    m_after_stage.insert(typeid(T).name());
    return *this;
  }

  template <typename T>
  Pass& set_before_stage() {
    m_before_stage.insert(typeid(T).name());
    return *this;
  }

 private:
  template <typename Func, typename... Args>
  void init_impl(Func&& func, std::tuple<Args...>*) {
    // 静态分析（这里可以使用 this，因为 init 是在对象创建时调用的）
    m_mutexes = Detail::merge_mutex_vectors(Detail::get_mutexes_for_type<Args>()...);
    m_preparers = Detail::get_preparers<Args...>();

    // 关键：值捕获 ID
    uint32_t passId = m_id;

    // 核心：m_binder 返回一个 lambda，而不是修改 this
    m_binder = [func = std::forward<Func>(func), passId](WorldBase& world) mutable -> CallType {
      // 捕获 passId 和 func，不捕获 this
      return [func, params = std::make_tuple(world.get_param<Args>(passId)...)]() mutable {
        std::apply(func, params);
      };
    };
  }

  template <typename Func, typename... Args>
  void init_helper(Func&& func, std::tuple<Args...>*) {
    // 静态分析权限和准备工作
    m_mutexes = Detail::merge_mutex_vectors(Detail::get_mutexes_for_type<Args>()...);
    m_preparers = Detail::get_preparers<Args...>();

    auto shared_func = std::make_shared<std::decay_t<Func>>(std::forward<Func>(func));

    m_binder = [shared_func, this](WorldBase& world) mutable {
      m_execute = [shared_func, ... params = world.get_param<Args>(m_id)]() mutable {
        (*shared_func)(params...);
      };
    };
  }

  static inline uint32_t sId = 0;
  uint32_t m_id = sId++;

  stl::string m_name;
  bool m_repeat = true;
  uint32_t m_priority = 0;

  stl::string m_stage;
  stl::unordered_set<stl::string> m_before_stage;
  stl::unordered_set<stl::string> m_after_stage;

  CallType m_execute;
  std::function<CallType(WorldBase&)> m_binder;

  stl::vector<Mutex> m_mutexes;
  stl::vector<std::function<void(WorldBase&)>> m_preparers;

  friend class World;
};
}  // namespace fe::engine::ecs