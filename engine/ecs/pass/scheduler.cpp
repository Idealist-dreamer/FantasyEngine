#include "scheduler.h"

namespace fe::engine::ecs {
using namespace fe::engine::stl;

struct PassInfo {
  shared_ptr<Pass> pass;
  tf::Task flowTask;
};

struct AccessInfo {
  unordered_map<AccessType, vector<PassId>> typeToPassIds;
  vector<unique_ptr<tf::Semaphore>> semaphores;
};

struct Scheduler::Impl {
  tf::Executor executor;
  tf::Taskflow taskflow;
  unordered_map<PassId, PassInfo> infoMap;

  vector<PassId> exclusivePasses;
};

Scheduler::Scheduler() {
  m_pImpl = stl::make_unique<Impl>();
}

Scheduler::~Scheduler() {
  d()->executor.wait_for_all();
}

void Scheduler::compile(const vector<shared_ptr<Pass>>& passes) {
  d()->taskflow.clear();
  d()->infoMap.clear();

  if (passes.empty())
    return;

  d()->infoMap.reserve(passes.size());

  for (auto pass : passes) {
    if (pass->id().null())
      continue;

    PassInfo info;
    info.pass = pass;
    info.flowTask = d()->taskflow.emplace([pass]() { pass->execute(); }).name(pass->name().c_str());

    d()->infoMap.insert({pass->id(), std::move(info)});
  }

  auto count = passes.size();

  for (size_t i = 0; i < count; ++i) {
    for (size_t j = i + 1; j < count; ++j) {
      auto& passA = passes[i];
      auto& passB = passes[j];

      if (passA->id().null() || passB->id().null())
        continue;

      auto& taskA = d()->infoMap[passA->id()].flowTask;
      auto& taskB = d()->infoMap[passB->id()].flowTask;

      bool conflictFound = false;

      const auto& predsB = passB->predecessors();
      for (auto predId : predsB) {
        if (predId == passA->id()) {
          conflictFound = true;
          break;
        }
      }

      if (conflictFound) {
        taskA.precede(taskB);
        continue;
      }

      const auto& mutexsA = passA->mutexs();
      const auto& mutexsB = passB->mutexs();

      for (const auto& mutA : mutexsA) {
        if (conflictFound)
          break;

        for (const auto& mutB : mutexsB) {
          if (mutA.isConflict(mutB)) {
            conflictFound = true;
            break;
          }
        }
      }

      if (conflictFound) {
        taskA.precede(taskB);
      }
    }
  }
}

void Scheduler::execute() {
  try {
    d()->executor.run(d()->taskflow).wait();
  } catch (const std::exception& e) {
    FE_ERROR("Scheduler execution failed: %s", e.what());
    throw;  // Rethrow for caller to handle
  }
}

void Scheduler::dumpGraph(const string& path) {
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