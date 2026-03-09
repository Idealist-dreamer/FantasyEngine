#include "world.h"

#include "paramPrepare.h"

#include <taskflow/taskflow.hpp>

namespace fe::engine {
struct World::Impl {
  stl::deque<Pass> m_default_passes;

  tf::Executor executor;
  tf::Taskflow setup_taskflow;
  tf::Taskflow run_taskflow;

  stl::map<stl::string, stl::shared_ptr<System>> sys_map;
};

World::World() {
  FE_DECLARE_PRIVATE_INIT
  d()->m_default_passes.push_back(Pass::create_start<stage::Init>("World_Init", [this]() { Init(); }, Priority::First));
  d()->m_default_passes.push_back(Pass::create_start<stage::PreStartup>("World_PreStartup", [this]() { PreStartup(); }, Priority::First));
  d()->m_default_passes.push_back(Pass::create_start<stage::Startup>("World_Startup", [this]() { Startup(); }, Priority::First));
  d()->m_default_passes.push_back(Pass::create_start<stage::PostStartup>("World_PostStartup", [this]() { PostStartup(); }, Priority::First));

  d()->m_default_passes.push_back(Pass::create_update<stage::First>("World_First", [this]() { First(); }, Priority::First));
  d()->m_default_passes.push_back(Pass::create_update<stage::PreUpdate>("World_PreUpdate", [this]() { PreUpdate(); }, Priority::First));
  d()->m_default_passes.push_back(Pass::create_update<stage::Update>("World_Update", [this]() { Update(); }, Priority::First));
  d()->m_default_passes.push_back(Pass::create_update<stage::PostUpdate>("World_PostUpdate", [this]() { PostUpdate(); }, Priority::First));
  d()->m_default_passes.push_back(Pass::create_update<stage::Last>("World_Last", [this]() { Last(); }, Priority::First));
  d()->m_default_passes.push_back(Pass::create_update<stage::Cleanup>("World_Cleanup", [this]() { Cleanup(); }, Priority::First));
}
World::~World() {}

void World::add_system(stl::shared_ptr<System> sys) {
  auto& sys_map = d()->sys_map;

  FE_ASSERT(sys_map.find(sys->m_name) == sys_map.end());
  sys_map.insert({sys->m_name, sys});
}

void World::compile() {
  auto& sys_map = d()->sys_map;
  auto& setup_taskflow = d()->setup_taskflow;
  auto& run_taskflow = d()->run_taskflow;

  stl::unordered_set<stl::string> pass_name_set;
  stl::vector<Pass*> setup_passes;
  stl::vector<Pass*> run_passes;

  for (auto& pass : d()->m_default_passes) {
    if (pass.m_repeat) {
      run_passes.push_back(&pass);
    } else {
      setup_passes.push_back(&pass);
    }
  }

  auto visitor = Visitor<WorldBase>(*this);

  for (auto& [name, sys] : sys_map) {
    if (sys->init(visitor)) {
      for (auto& pass : sys->m_passes) {
        FE_ASSERT(pass_name_set.find(pass.m_name) == pass_name_set.end());
        pass_name_set.insert(pass.m_name);

        if (pass.m_repeat) {
          run_passes.push_back(&pass);
        } else {
          setup_passes.push_back(&pass);
        }
      }
    }
  }

  for (auto pass : setup_passes) {
    for (auto& preparer : pass->m_preparers) {
      preparer(*this);
    }
  }
  for (auto pass : run_passes) {
    for (auto& preparer : pass->m_preparers) {
      preparer(*this);
    }
  }

  for (auto pass : setup_passes) {
    pass->m_execute = pass->m_binder(*this);
  }
  for (auto pass : run_passes) {
    pass->m_execute = pass->m_binder(*this);
  }

  setup_taskflow.clear();
  run_taskflow.clear();

  if (run_passes.empty() && setup_passes.empty()) {
    return;
  }

  // Create Stage barrier tasks to ensure dependency chain doesn't break on empty stages
  auto create_stage_barriers = [](tf::Taskflow& tf, const stl::vector<Pass*>& passes) -> stl::map<stage::StageHash, stl::pair<tf::Task, tf::Task>> {
    stl::map<stage::StageHash, stl::pair<tf::Task, tf::Task>> barriers;
    stl::unordered_set<stage::StageHash> used_stages;

    stl::vector<stage::StageHash> queue;
    for (auto pass : passes) {
      if (used_stages.insert(pass->m_stage).second) {
        queue.push_back(pass->m_stage);
      }
    }

    for (size_t head = 0; head < queue.size(); ++head) {
      auto current = queue[head];
      for (auto next : stage::g_stage_after_map[current]) {
        if (used_stages.insert(next).second) {
          queue.push_back(next);
        }
      }
    }

    queue.assign(used_stages.begin(), used_stages.end());
    for (size_t head = 0; head < queue.size(); ++head) {
      auto current = queue[head];
      for (auto prev : stage::g_stage_before_map[current]) {
        if (used_stages.insert(prev).second) {
          queue.push_back(prev);
        }
      }
    }

    stl::string begin_str = "_Begin";
    stl::string end_str = "_End";

    // Create barrier task for each stage
    for (auto stage_hash : used_stages) {
      barriers[stage_hash].first = tf.emplace([]() {}).name((stage::g_stage_name_registry[stage_hash] + begin_str).c_str());
      barriers[stage_hash].second = tf.emplace([]() {}).name((stage::g_stage_name_registry[stage_hash] + end_str).c_str());
      barriers[stage_hash].first.precede(barriers[stage_hash].second);
    }

    for (auto stage_hash : used_stages) {
      auto& before_stages = stage::g_stage_before_map[stage_hash];
      for (auto& before_hash : before_stages) {
        if (barriers.find(before_hash) != barriers.end()) {
          barriers[before_hash].second.precede(barriers[stage_hash].first);
        }
      }

      auto& after_stages = stage::g_stage_after_map[stage_hash];
      for (auto& after_hash : after_stages) {
        if (barriers.find(after_hash) != barriers.end()) {
          barriers[after_hash].first.succeed(barriers[stage_hash].second);
        }
      }
    }

    return barriers;
  };

  stl::map<stage::StageHash, stl::pair<tf::Task, tf::Task>> setup_barriers = create_stage_barriers(setup_taskflow, setup_passes);
  stl::map<stage::StageHash, stl::pair<tf::Task, tf::Task>> run_barriers = create_stage_barriers(run_taskflow, run_passes);

  // Create Pass tasks
  stl::map<Pass*, tf::Task> task_node_map;
  for (auto pass : setup_passes) {
    task_node_map[pass] = setup_taskflow.emplace([pass, this]() { pass->m_execute(); }).name(pass->m_name.c_str());
  }
  for (auto pass : run_passes) {
    task_node_map[pass] = run_taskflow.emplace([pass, this]() { pass->m_execute(); }).name(pass->m_name.c_str());
  }

  // Link Pass tasks to their corresponding Stage barriers
  auto link_pass_to_barriers = [](stl::map<Pass*, tf::Task>& task_map, stl::map<stage::StageHash, stl::pair<tf::Task, tf::Task>>& barriers,
                                  const stl::vector<Pass*>& passes) {
    for (auto pass : passes) {
      auto& task = task_map[pass];
      auto stage_hash = pass->m_stage;

      // Pass must execute after its Stage barrier
      if (barriers.find(stage_hash) != barriers.end()) {
        barriers[stage_hash].first.precede(task);
        barriers[stage_hash].second.succeed(task);
      }
    }
  };

  link_pass_to_barriers(task_node_map, setup_barriers, setup_passes);
  link_pass_to_barriers(task_node_map, run_barriers, run_passes);

  auto is_stage_reachable = [](stage::StageHash start, stage::StageHash target) -> bool {
    if (start == target)
      return false;
    stl::unordered_set<stage::StageHash> visited;
    stl::vector<stage::StageHash> queue{start};
    visited.insert(start);
    for (size_t head = 0; head < queue.size(); ++head) {
      auto curr = queue[head];
      if (curr == target)
        return true;
      auto it = stage::g_stage_after_map.find(curr);
      if (it != stage::g_stage_after_map.end()) {
        for (auto next : it->second) {
          if (visited.insert(next).second)
            queue.push_back(next);
        }
      }
    }
    return false;
  };

  auto mutex_pass_fun = [&task_node_map, &is_stage_reachable](const stl::vector<Pass*>& pass_array) {
    auto count = pass_array.size();
    for (size_t i = 0; i < count; ++i) {
      for (size_t j = i + 1; j < count; ++j) {
        auto pass_a = pass_array[i];
        auto pass_b = pass_array[j];

        auto& task_a = task_node_map[pass_a];
        auto& task_b = task_node_map[pass_b];

        bool is_conflict = false;
        for (auto& mutex_a : pass_a->m_mutexes) {
          for (auto& mutex_b : pass_b->m_mutexes) {
            if (mutex_a.is_conflict(mutex_b)) {
              is_conflict = true;
              break;
            }
          }
          if (is_conflict)
            break;
        }

        if (is_conflict) {
          bool a_before_b = is_stage_reachable(pass_a->m_stage, pass_b->m_stage);
          bool b_before_a = is_stage_reachable(pass_b->m_stage, pass_a->m_stage);

          if (!a_before_b && !b_before_a) {
            if (pass_a->m_priority <= pass_b->m_priority) {
              task_a.precede(task_b);
            } else {
              task_b.precede(task_a);
            }
          }
        }
      }
    }
  };

  mutex_pass_fun(setup_passes);
  mutex_pass_fun(run_passes);
}

void World::setup() {
  try {
    d()->executor.run(d()->setup_taskflow).wait();
  } catch (const std::exception& e) {
    FE_ERROR("Scheduler execution failed: %s", e.what());
    throw;  // Rethrow for caller to handle
  }
}

void World::run() {
  try {
    d()->executor.run(d()->run_taskflow).wait();
  } catch (const std::exception& e) {
    FE_ERROR("Scheduler execution failed: %s", e.what());
    throw;  // Rethrow for caller to handle
  }
}

void World::dump_graph(const stl::string& path) {
  std::filesystem::path base_path(path.c_str());

  if (base_path.has_parent_path()) {
    std::filesystem::create_directories(base_path.parent_path());
  }

  auto directory = base_path.parent_path();
  auto stem = base_path.stem().string();

  std::filesystem::path setup_path = directory / (stem + "_setup.dot");
  std::filesystem::path run_path = directory / (stem + "_run.dot");

  {
    std::ofstream ofs(setup_path);
    if (ofs) {
      d()->setup_taskflow.dump(ofs);
    }
  }

  {
    std::ofstream ofs(run_path);
    if (ofs) {
      d()->run_taskflow.dump(ofs);
    }
  }
}

void World::Init() {}
void World::PreStartup() {}
void World::Startup() {}
void World::PostStartup() {}

void World::First() {}
void World::PreUpdate() {}
void World::Update() {}
void World::PostUpdate() {}

void World::Last() {}
void World::Cleanup() {
  for (auto& [tid, swap] : m_event_swap) {
    swap(*this);
  }
  for (auto& [passId, ecb] : m_entity_command_buffers) {
    for (auto& entity : ecb.m_entity_map) {
      if (entity == entt::null) {
        entity = m_registry.create();
      }
    }
    for (auto& e : ecb.m_destroyed_entities) {
      m_registry.destroy(e);
    }
    ecb.m_destroyed_entities.clear();
  }
}

}  // namespace fe::engine
