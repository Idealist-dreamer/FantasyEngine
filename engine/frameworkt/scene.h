#pragma once

#include "system.h"
#include <taskflow/taskflow.hpp>

namespace fe::engine {

class Scene {
public:
    Scene();
    ~Scene() = default;

    // 添加系统
    void add_system(stl::shared_ptr<System> sys);

    // 编译：收集所有 Pass，构建任务图
    void compile();

    // 执行一次性设置任务流
    void setup();

    // 执行每帧更新任务流
    void run();

    // 导出任务图 DOT 文件
    void dump_graph(const stl::string& path);

    // 序列化场景（仅实体和组件）
    void save(const stl::string& path);
    void load(const stl::string& path);

    // 访问黑板
    Blackboard& blackboard() { return m_blackboard; }
    const Blackboard& blackboard() const { return m_blackboard; }

private:
    // 构建阶段屏障任务
    stl::map<stage::StageHash, stl::pair<tf::Task, tf::Task>>
    build_stage_barriers(tf::Taskflow& tf, const stl::vector<Pass*>& passes);

    // 判断阶段可达性
    bool is_stage_reachable(stage::StageHash from, stage::StageHash to) const;

    // 应用实体命令缓冲区（在 Cleanup 阶段调用）
    void apply_command_buffers();

    Blackboard m_blackboard;
    stl::vector<Pass> m_passes;                 // 内置 Pass + 系统 Pass
    stl::vector<stl::shared_ptr<System>> m_systems;

    tf::Executor m_executor;
    tf::Taskflow m_setup_tf;
    tf::Taskflow m_run_tf;
};

} // namespace fe::engine
