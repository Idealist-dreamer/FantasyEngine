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

void World::addSystem(stl::shared_ptr<System> sys) {
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

      if (passA->m_before.find(passB->m_name) != passA->m_before.end()) {
        taskA.succeed(taskB);
        break;
      }
      if (passB->m_before.find(passA->m_name) != passA->m_before.end()) {
        taskB.succeed(taskA);
        break;
      }

      if (passA->m_after.find(passB->m_name) != passA->m_after.end()) {
        taskA.precede(taskB);
        break;
      }
      if (passB->m_after.find(passA->m_name) != passA->m_after.end()) {
        taskB.precede(taskA);
        break;
      }

      bool isConflict = false;
      for (auto& mutexA : passA->m_mutexs) {
        for (auto& mutexB : passB->m_mutexs) {
          if (mutexA.isConflict(mutexB)) {
            isConflict = true;
            break;
          }
        }
        if (isConflict) {
          break;
        }
      }

      if (isConflict) {
        taskA.precede(taskB);
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

void World::dumpGraph(const stl::string& path) {
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