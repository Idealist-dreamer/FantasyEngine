// scene.h
#pragma once

#include "scene_context.h"
#include "pass.h"
#include <taskflow/taskflow.hpp>

namespace fe::engine {

class Scene {
 public:
  Scene() = default;

  void add_pass(Pass pass) { m_passes.push_back(std::move(pass)); }

  void compile();
  void update();

  SceneContext& context() { return m_context; }

 private:
  SceneContext m_context;
  stl::vector<Pass> m_passes;

  tf::Executor m_executor;
  tf::Taskflow m_taskflow;
};

}  // namespace fe::engine

// scene.cpp
#include "scene.h"

namespace fe::engine {

void Scene::compile() {
  // 1. 准备期：让所有 Pass 向 SceneContext 声明需要的数据和 Refresh 闭包
  for (auto& pass : m_passes) {
    if (pass.m_preparer) {
      pass.m_preparer(m_context);
    }
  }

  // 2. 绑定期：生成彻底摆脱 TypeID 查表运行开销的 Lambda
  for (auto& pass : m_passes) {
    pass.m_execute = pass.m_binder(m_context);
  }

  m_taskflow.clear();

  // 3. 构建有向无环图 (依赖判定依据为全新的二层互斥锁)
  stl::vector<tf::Task> tasks;
  for (auto& pass : m_passes) {
    tasks.push_back(m_taskflow.emplace([&pass]() { pass.m_execute(); }).name(pass.m_name));
  }

  for (size_t i = 0; i < m_passes.size(); ++i) {
    for (size_t j = i + 1; j < m_passes.size(); ++j) {
      if (m_passes[i].m_mutex.is_conflict(m_passes[j].m_mutex)) {
        // 根据 Stage 或顺序定义，将相互冲突的系统串行化
        tasks[i].precede(tasks[j]);
      }
    }
  }
}

void Scene::update() {
  // 极速并发执行所有的 System Passes
  m_executor.run(m_taskflow).wait();

  // 帧末执行注册好的清理工作（Event 交换，ECB 执行等）
  m_context.refresh();
}

}  // namespace fe::engine