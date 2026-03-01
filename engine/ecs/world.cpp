#include "world.h"

#include <taskflow/taskflow.hpp>

namespace fe::engine::ecs {
struct World::Impl {
  tf::Executor executor;
  tf::Taskflow taskflow;

  stl::map<stl::string, stl::shared_ptr<System>> sys_map;
};

World::World(){FE_DECLARE_PRIVATE_INIT}

World::~World() {}

void World::add_system(stl::shared_ptr<System> sys) {
  auto& sys_map = d()->sys_map;

  FE_ASSERT(sys_map.find(sys->m_name) == sys_map.end());
  sys_map.insert({sys->m_name, sys});
  sys->init(*this);
}

void World::compile() {
  auto& sys_map = d()->sys_map;
  auto& taskflow = d()->taskflow;

  stl::unordered_set<stl::string> pass_name_set;
  stl::vector<Pass*> pass_array;
  for (auto& [name, sys] : sys_map) {
    auto& passes = sys->m_passes;

    for (auto& pass : passes) {
      FE_ASSERT(pass_name_set.find(pass.m_name) == pass_name_set.end());
      pass_name_set.insert(pass.m_name);
      pass_array.push_back(&pass);
    }
  }

  for (auto preparer : detail::get_default_preparers()) {
    preparer(m_registry);
  }

  for (auto pass : pass_array) {
    for (auto& preparer : pass->m_preparers) {
      preparer(m_registry);
    }
  }

  taskflow.clear();

  if (pass_array.empty()) {
    return;
  }

  stl::map<Pass*, tf::Task> task_node_map;
  for (auto pass : pass_array) {
    task_node_map[pass] = taskflow.emplace([pass, this]() { pass->m_call(*this); }).name(pass->m_name.c_str());
  }

  auto count = pass_array.size();
  for (size_t i = 0; i < count; ++i) {
    for (size_t j = i + 1; j < count; ++j) {
      auto pass_a = pass_array[i];
      auto pass_b = pass_array[j];

      auto& task_a = task_node_map[pass_a];
      auto& task_b = task_node_map[pass_b];

      if (pass_a->m_before_passes.find(pass_b->m_name) != pass_a->m_before_passes.end() ||
          pass_b->m_after_passes.find(pass_a->m_name) != pass_b->m_after_passes.end()) {
        task_a.precede(task_b);
        continue;
      }
      if (pass_a->m_after_passes.find(pass_b->m_name) != pass_a->m_after_passes.end() ||
          pass_b->m_before_passes.find(pass_a->m_name) != pass_b->m_before_passes.end()) {
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
}

void World::run() {
  try {
    d()->executor.run(d()->taskflow).wait();
  } catch (const std::exception& e) {
    FE_ERROR("Scheduler execution failed: %s", e.what());
    throw;  // Rethrow for caller to handle
  }
}

void World::dump_graph(const stl::string& path) {
  std::filesystem::path fs_path(path.c_str());
  if (fs_path.has_parent_path()) {
    std::filesystem::create_directories(fs_path.parent_path());
  }

  std::ofstream ofs(fs_path);

  if (ofs.is_open()) {
    d()->taskflow.dump(ofs);
    ofs.close();
  } else {
    // 这里可以根据你的项目规范抛出异常或记录日志
    // RE_LOG_ERROR(L"Failed to open file for dumping graph: " + path);
  }
}

}  // namespace fe::engine::ecs
