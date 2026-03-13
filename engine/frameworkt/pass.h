#pragma once

#include "param_ops.h"
#include "stage.h"
#include "foundation/utility/meta.h"

namespace fe::engine {

// Pass：可执行单元，携带名称、阶段、优先级、访问树、准备函数和执行函数
class Pass {
public:
    Pass(stl::string name, bool repeat = true, uint32_t priority = Priority::Low)
        : m_name(std::move(name)), m_repeat(repeat), m_priority(priority) {}

    // 静态工厂：创建一次性启动Pass
    template <typename StageT, typename Func>
    static Pass create_start(stl::string name, Func&& func, uint32_t priority = Priority::First) {
        Pass pass(std::move(name), false, priority);
        pass.set_stage<StageT>();
        pass.bind(std::forward<Func>(func));
        return pass;
    }

    // 静态工厂：创建每帧更新Pass
    template <typename StageT, typename Func>
    static Pass create_update(stl::string name, Func&& func, uint32_t priority = Priority::Low) {
        Pass pass(std::move(name), true, priority);
        pass.set_stage<StageT>();
        pass.bind(std::forward<Func>(func));
        return pass;
    }

    // 绑定可调用对象，自动推导参数类型并构建访问树、准备函数和执行函数
    template <typename Func>
    void bind(Func&& func) {
        using FuncType = std::remove_cvref_t<Func>;
        using ArgsTuple = typename meta::function_traits<FuncType>::args_tuple;
        bind_impl(std::forward<Func>(func), static_cast<ArgsTuple*>(nullptr));
    }

    // 设置阶段
    template <typename StageT>
    Pass& set_stage() {
        m_stage = stage::get_stage_hash<StageT>();
        return *this;
    }

    // 冲突检测
    bool is_conflict(const Pass& other) const { return m_access.is_conflict(other.m_access); }

    // 准备黑板数据（在编译阶段调用）
    void prepare(Blackboard& bb) const { if (m_prepare) m_prepare(bb); }

    // 执行（在运行时调用）
    void execute(Blackboard& bb) const { if (m_execute) m_execute(bb); }

    // 公共成员（供Scene访问）
    stl::string m_name;
    bool m_repeat = true;
    uint32_t m_priority = Priority::Low;
    stage::StageHash m_stage = 0;
    Access m_access;

private:
    template <typename Func, typename... Args>
    void bind_impl(Func&& func, std::tuple<Args...>*) {
        // 构建访问树
        m_access = ParamOps::build_access<Args...>();

        // 构建准备函数
        m_prepare = [](Blackboard& bb) {
            ParamOps::prepare<Args...>(bb);
        };

        // 构建执行函数（类型擦除）
        m_execute = [f = std::forward<Func>(func)](Blackboard& bb) mutable {
            auto params = ParamOps::fetch<Args...>(bb);
            std::apply(f, std::move(params));
        };
    }

    std::function<void(Blackboard&)> m_prepare;
    std::function<void(Blackboard&)> m_execute;
};

} // namespace fe::engine
