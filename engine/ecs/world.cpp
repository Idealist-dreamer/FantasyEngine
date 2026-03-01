#include "world.h"

#include <taskflow/taskflow.hpp>

namespace fe::engine::ecs {
struct World::Impl {
  tf::Executor executor;
  tf::Taskflow taskflow;

  stl::map<stl::string, stl::shared_ptr<System>> sysMap;
};

World::World(){FE_DECLARE_PRIVATE_INIT}

World::~World() {}

void World::add_system(stl::shared_ptr<System> sys) {
  auto& sysMap = d()->sysMap;

  FE_ASSERT(sysMap.find(sys->m_name) == sysMap.end());
  sysMap.insert({sys->m_name, sys});
  sys->init(*this);
}

void World::compile() {
  auto& sysMap = d()->sysMap;
  auto& taskflow = d()->taskflow;

  stl::unordered_set<stl::string> passNameSet;
  stl::vector<Pass*> passArray;
  for (auto& [name, sys] : sysMap) {
    auto& passes = sys->m_passes;

    for (auto& pass : passes) {
      FE_ASSERT(passNameSet.find(pass.m_name) == passNameSet.end());
      passNameSet.insert(pass.m_name);
      passArray.push_back(&pass);
    }
  }

  for (auto preparer : detail::get_default_preparers()) {
    preparer(m_registry);
  }

  for (auto pass : passArray) {
    for (auto& preparer : pass->m_preparers) {
      preparer(m_registry);
    }
  }

  taskflow.clear();

  if (passArray.empty()) {
    return;
  }

  stl::map<Pass*, tf::Task> taskNodeMap;
  for (auto pass : passArray) {
    taskNodeMap[pass] = taskflow.emplace([pass, this]() { pass->m_call(*this); }).name(pass->m_name.c_str());
  }

  auto count = passArray.size();
  for (size_t i = 0; i < count; ++i) {
    for (size_t j = i + 1; j < count; ++j) {
      auto passA = passArray[i];
      auto passB = passArray[j];

      auto& taskA = taskNodeMap[passA];
      auto& taskB = taskNodeMap[passB];

      if (passA->m_executeBefore.find(passB->m_name) != passA->m_executeBefore.end() ||
          passB->m_executeAfter.find(passA->m_name) != passB->m_executeAfter.end()) {
        taskA.precede(taskB);
        continue;
      }
      if (passA->m_executeAfter.find(passB->m_name) != passA->m_executeAfter.end() ||
          passB->m_executeBefore.find(passA->m_name) != passB->m_executeBefore.end()) {
        taskA.succeed(taskB);
        continue;
      }

      bool isConflict = false;
      for (auto& mutexA : passA->m_mutexs) {
        for (auto& mutexB : passB->m_mutexs) {
          if (mutexA.is_conflict(mutexB)) {
            isConflict = true;
            break;
          }
        }
        if (isConflict) {
          break;
        }
      }

      if (isConflict) {
        if (passA->m_priority <= passB->m_priority) {
          taskA.precede(taskB);
        } else {
          taskB.precede(taskA);
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
  std::filesystem::path fsPath(path.c_str());
  if (fsPath.has_parent_path()) {
    std::filesystem::create_directories(fsPath.parent_path());
  }

  std::ofstream ofs(fsPath);

  if (ofs.is_open()) {
    d()->taskflow.dump(ofs);
    ofs.close();
  } else {
    // 这里可以根据你的项目规范抛出异常或记录日志
    // RE_LOG_ERROR(L"Failed to open file for dumping graph: " + path);
  }
}

}  // namespace fe::engine::ecs