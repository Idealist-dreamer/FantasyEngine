#pragma once

#include "blackboard.h"
#include "pass.h"
#include "system.h"
#include <taskflow/taskflow.hpp>

namespace fe::engine {

// Scene: Task-Graph scheduler (formerly World)
// Manages passes, builds dependency graph, executes with taskflow
class Scene {
public:
    Scene();
    ~Scene() = default;

    // Add a system
    void add_system(stl::shared_ptr<System> sys);

    // Compile all passes into task graph
    void compile();

    // Run setup taskflow (one-time initialization)
    void setup();

    // Run main taskflow (per-frame)
    void run();

    // Dump task graph to file
    void dump_graph(const stl::string& path);

    // Serialization
    void save(const stl::string& path);
    void load(const stl::string& path);

    // Access blackboard
    Blackboard& blackboard() { return m_blackboard; }
    const Blackboard& blackboard() const { return m_blackboard; }

private:
    // Build stage barrier tasks
    stl::map<stage::StageHash, stl::pair<tf::Task, tf::Task>> 
    build_stage_barriers(tf::Taskflow& tf, const stl::vector<Pass*>& passes);

    // Check if stage is reachable from another
    bool is_stage_reachable(stage::StageHash from, stage::StageHash to) const;

    // Apply entity command buffers
    void apply_command_buffers();

    Blackboard m_blackboard;
    stl::vector<Pass> m_passes;
    stl::vector<stl::shared_ptr<System>> m_systems;
    
    tf::Executor m_executor;
    tf::Taskflow m_setup_tf;
    tf::Taskflow m_run_tf;
};

} // namespace fe::engine
