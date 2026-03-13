#pragma once

#include "pass.h"

namespace fe::engine {

// 前向声明
class Archive;

// 系统基类：用户需继承并实现虚函数
class System {
public:
    explicit System(stl::string name) : m_name(std::move(name)) {}
    virtual ~System() = default;

    // 初始化，返回 false 表示跳过该系统
    virtual bool init(Blackboard& bb) = 0;

    // 序列化钩子
    virtual void save(Blackboard& bb, Archive& ar) {}
    virtual void load(Blackboard& bb, Archive& ar) {}

protected:
    stl::string m_name;
    stl::vector<Pass> m_passes;   // 系统注册的 Pass

    friend class Scene;
};

// 辅助宏：快速添加 Pass
#define FE_SYS_PASS(ClassName, FuncName, Stage) \
    m_passes.push_back(Pass::create_update<Stage>(#FuncName, [this](auto&&... args) { \
        this->FuncName(std::forward<decltype(args)>(args)...); \
    }))

} // namespace fe::engine
