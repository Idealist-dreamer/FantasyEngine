#include "world.h"

#include "paramDetail.h"

#include <taskflow/taskflow.hpp>

namespace fe::engine::ecs {
struct World::Impl {
  stl::vector<Pass> m_default_passes;

  tf::Executor executor;
  tf::Taskflow setup_taskflow;
  tf::Taskflow run_taskflow;

  stl::map<stl::string, stl::shared_ptr<System>> sys_map;
};

World::World() {
  FE_DECLARE_PRIVATE_INIT
  d()->m_default_passes.push_back(Pass::create_start("Wolrd_PreStartup", [this]() { PreStartup(); }, Priority::First));
  d()->m_default_passes.push_back(Pass::create_start("Wolrd_Startup", [this]() { Startup(); }, Priority::First));
  d()->m_default_passes.push_back(Pass::create_start("Wolrd_PostStartup", [this]() { PostStartup(); }, Priority::First));

  d()->m_default_passes.push_back(Pass::create_update("Wolrd_PreUpdate", [this]() { PreUpdate(); }, Priority::First));
  d()->m_default_passes.push_back(Pass::create_update("Wolrd_Update", [this]() { Update(); }, Priority::First));
  d()->m_default_passes.push_back(Pass::create_update("Wolrd_PostUpdate", [this]() { PostUpdate(); }, Priority::First));
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

  for (auto& [name, sys] : sys_map) {
    sys->setWorld(*this);

    if (sys->init()) {
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

  for (auto preparer : Detail::get_default_preparers()) {
    preparer(*this);
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
    pass->m_binder(*this);
  }
  for (auto pass : run_passes) {
    pass->m_binder(*this);
  }

  setup_taskflow.clear();
  run_taskflow.clear();

  if (run_passes.empty() && setup_passes.empty()) {
    return;
  }

  stl::map<Pass*, tf::Task> task_node_map;
  for (auto pass : setup_passes) {
    task_node_map[pass] = setup_taskflow.emplace([pass, this]() { pass->m_execute(); }).name(pass->m_name.c_str());
  }
  for (auto pass : run_passes) {
    task_node_map[pass] = run_taskflow.emplace([pass, this]() { pass->m_execute(); }).name(pass->m_name.c_str());
  }

  auto mutex_pass_fun = [&task_node_map](const stl::vector<Pass*>& pass_array) {
    auto count = pass_array.size();
    for (size_t i = 0; i < count; ++i) {
      for (size_t j = i + 1; j < count; ++j) {
        auto pass_a = pass_array[i];
        auto pass_b = pass_array[j];

        auto& task_a = task_node_map[pass_a];
        auto& task_b = task_node_map[pass_b];

        if (pass_a->m_before_stage.find(pass_b->m_name) != pass_a->m_before_stage.end() ||
            pass_b->m_after_stage.find(pass_a->m_name) != pass_b->m_after_stage.end()) {
          task_a.precede(task_b);
          continue;
        }
        if (pass_a->m_after_stage.find(pass_b->m_name) != pass_a->m_after_stage.end() ||
            pass_b->m_before_stage.find(pass_a->m_name) != pass_b->m_before_stage.end()) {
          task_a.succeed(task_b);
          continue;
        }

        bool is_conflict = false;
        for (auto& mutex_a : pass_a->m_mutexes) {
          for (auto& mutex_b : pass_b->m_mutexes) {
            if (mutex_a.is_conflict(mutex_b)) {
              is_conflict = true;
              break;
            }
          }
          if (is_conflict) {
            break;
          }
        }

        if (is_conflict) {
          if (pass_a->m_priority <= pass_b->m_priority) {
            task_a.precede(task_b);
          } else {
            task_b.precede(task_a);
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
    next_frame();
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

void World::PreStartup() {}
void World::Startup() {}
void World::PostStartup() {}

void World::PreUpdate() {}
void World::Update() {}
void World::PostUpdate() {}

void World::Cleanup() {}

}  // namespace fe::engine::ecs
