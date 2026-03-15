#include "scene.h"
#include "native_type.h"
#include "access.h"

#include "foundation/utility/assert.h"

#include <taskflow/taskflow.hpp>
#include <fstream>
#include <filesystem>

namespace fe::engine {
struct Scene::Impl {
  stl::string m_name;

  stl::deque<Pass> m_default_passes;
  stl::vector<stl::shared_ptr<System>> m_systems;

  tf::Executor m_executor;
  tf::Taskflow m_setup_taskflow;
  tf::Taskflow m_run_taskflow;
};

Scene::Scene() {
  FE_DECLARE_PRIVATE_INIT
  d()->m_default_passes.push_back(Pass::create_start<stage::Init>(
      "Scene_Init", [this]() { Init(); }, Priority::First));
  d()->m_default_passes.push_back(Pass::create_start<stage::PreStartup>(
      "Scene_PreStartup", [this]() { PreStartup(); }, Priority::First));
  d()->m_default_passes.push_back(Pass::create_start<stage::Startup>(
      "Scene_Startup", [this]() { Startup(); }, Priority::First));
  d()->m_default_passes.push_back(Pass::create_start<stage::PostStartup>(
      "Scene_PostStartup", [this]() { PostStartup(); }, Priority::First));

  d()->m_default_passes.push_back(Pass::create_update<stage::First>(
      "Scene_First", [this]() { First(); }, Priority::First));
  d()->m_default_passes.push_back(Pass::create_update<stage::PreUpdate>(
      "Scene_PreUpdate", [this]() { PreUpdate(); }, Priority::First));
  d()->m_default_passes.push_back(Pass::create_update<stage::Update>(
      "Scene_Update", [this]() { Update(); }, Priority::First));
  d()->m_default_passes.push_back(Pass::create_update<stage::PostUpdate>(
      "Scene_PostUpdate", [this]() { PostUpdate(); }, Priority::First));
  d()->m_default_passes.push_back(Pass::create_update<stage::Last>(
      "Scene_Last", [this]() { Last(); }, Priority::First));
  d()->m_default_passes.push_back(Pass::create_update<stage::Cleanup>(
      "Scene_Cleanup", [this]() { Cleanup(); }, Priority::First));
}

Scene::~Scene() {}

void Scene::add_system(stl::shared_ptr<System> sys) {
  d()->m_systems.push_back(sys);
}

void Scene::compile() {
  auto& sys_map = d()->m_systems;
  auto& setup_taskflow = d()->m_setup_taskflow;
  auto& run_taskflow = d()->m_run_taskflow;

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

  for (auto& sys : sys_map) {
    if (sys->init(*this)) {
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
      preparer(m_blackboard);
    }
  }
  for (auto pass : run_passes) {
    for (auto& preparer : pass->m_preparers) {
      preparer(m_blackboard);
    }
  }

  for (auto pass : setup_passes) {
    pass->m_execute = pass->m_binder(m_blackboard);
  }
  for (auto pass : run_passes) {
    pass->m_execute = pass->m_binder(m_blackboard);
  }

  setup_taskflow.clear();
  run_taskflow.clear();

  if (run_passes.empty() && setup_passes.empty()) {
    return;
  }

  auto create_stage_barriers = [](tf::Taskflow& tf,
                                  const stl::vector<Pass*>& passes)
      -> stl::map<stage::StageHash, stl::pair<tf::Task, tf::Task>> {
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

    for (auto stage_hash : used_stages) {
      barriers[stage_hash].first = tf.emplace([]() {}).name(
          (stage::g_stage_name_registry[stage_hash] + begin_str).c_str());
      barriers[stage_hash].second = tf.emplace([]() {}).name(
          (stage::g_stage_name_registry[stage_hash] + end_str).c_str());
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

  stl::map<stage::StageHash, stl::pair<tf::Task, tf::Task>> setup_barriers =
      create_stage_barriers(setup_taskflow, setup_passes);
  stl::map<stage::StageHash, stl::pair<tf::Task, tf::Task>> run_barriers =
      create_stage_barriers(run_taskflow, run_passes);

  stl::map<Pass*, tf::Task> task_node_map;
  for (auto pass : setup_passes) {
    task_node_map[pass] =
        setup_taskflow.emplace([pass]() { pass->m_execute(); })
            .name(pass->m_name.c_str());
  }
  for (auto pass : run_passes) {
    task_node_map[pass] = run_taskflow.emplace([pass]() { pass->m_execute(); })
                              .name(pass->m_name.c_str());
  }

  auto link_pass_to_barriers =
      [](stl::map<Pass*, tf::Task>& task_map,
         stl::map<stage::StageHash, stl::pair<tf::Task, tf::Task>>& barriers,
         const stl::vector<Pass*>& passes) {
        for (auto pass : passes) {
          auto& task = task_map[pass];
          auto stage_hash = pass->m_stage;
          if (barriers.find(stage_hash) != barriers.end()) {
            barriers[stage_hash].first.precede(task);
            barriers[stage_hash].second.succeed(task);
          }
        }
      };

  link_pass_to_barriers(task_node_map, setup_barriers, setup_passes);
  link_pass_to_barriers(task_node_map, run_barriers, run_passes);

  auto is_stage_reachable = [](stage::StageHash start,
                               stage::StageHash target) -> bool {
    if (start == target) return false;
    stl::unordered_set<stage::StageHash> visited;
    stl::vector<stage::StageHash> queue{start};
    visited.insert(start);
    for (size_t head = 0; head < queue.size(); ++head) {
      auto curr = queue[head];
      if (curr == target) return true;
      auto it = stage::g_stage_after_map.find(curr);
      if (it != stage::g_stage_after_map.end()) {
        for (auto next : it->second) {
          if (visited.insert(next).second) queue.push_back(next);
        }
      }
    }
    return false;
  };

  auto access_pass_fun = [&task_node_map, &is_stage_reachable](
                             const stl::vector<Pass*>& pass_array) {
    auto count = pass_array.size();
    for (size_t i = 0; i < count; ++i) {
      for (size_t j = i + 1; j < count; ++j) {
        auto pass_a = pass_array[i];
        auto pass_b = pass_array[j];

        auto& task_a = task_node_map[pass_a];
        auto& task_b = task_node_map[pass_b];

        // 使用新 AccessInfo 图判定替代旧版 mutex 遍历
        if (pass_a->m_access.is_conflict(pass_b->m_access)) {
          bool a_before_b =
              is_stage_reachable(pass_a->m_stage, pass_b->m_stage);
          bool b_before_a =
              is_stage_reachable(pass_b->m_stage, pass_a->m_stage);

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

  access_pass_fun(setup_passes);
  access_pass_fun(run_passes);
}

void Scene::setup() {
  try {
    d()->m_executor.run(d()->m_setup_taskflow).wait();
  } catch (const std::exception& e) {
    FE_ERROR("Scheduler execution failed: %s", e.what());
    throw;
  }
}

void Scene::run() {
  try {
    d()->m_executor.run(d()->m_run_taskflow).wait();
  } catch (const std::exception& e) {
    FE_ERROR("Scheduler execution failed: %s", e.what());
    throw;
  }
}

void Scene::dump_graph(const stl::string& path) {
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
      d()->m_setup_taskflow.dump(ofs);
    }
  }

  {
    std::ofstream ofs(run_path);
    if (ofs) {
      d()->m_run_taskflow.dump(ofs);
    }
  }
}

bool is_json(const stl::string& path) {
  return path.size() >= 5 && path.compare(path.size() - 5, 5, ".json") == 0;
}

void Scene::save(const stl::string& path) {
  std::ofstream os(path.c_str(), std::ios::binary);

  if (!os) {
    FE_ERROR("Failed to open file for saving: %s", path.c_str());
    return;
  }

  Registry* reg = m_blackboard.try_get<Registry>();

  if (is_json(path)) {
    cereal::JSONOutputArchive concreteAr(os);
    Archive archive(&concreteAr, reg);

    archive(FE_MAKE_NVP(d()->m_name));
    archive.entities();

    for (auto& sys : d()->m_systems) sys->save(*this, archive);
  } else {
    cereal::BinaryOutputArchive concreteAr(os);
    Archive archive(&concreteAr, reg);

    archive(FE_MAKE_NVP(d()->m_name));
    archive.entities();

    for (auto& sys : d()->m_systems) sys->save(*this, archive);
  }

  FE_DEBUGPRINT("Scene saved to: %s", path.c_str());
}

void Scene::load(const stl::string& path) {
  std::ifstream is(path.c_str(), std::ios::binary);

  if (!is) {
    FE_ERROR("Failed to open file for loading: %s", path.c_str());
    return;
  }

  // 重置 Registry
  m_blackboard.emplace_or_replace<Registry>();
  Registry* reg = m_blackboard.try_get<Registry>();

  if (is_json(path)) {
    cereal::JSONInputArchive concreteAr(is);
    Archive archive(&concreteAr, reg);

    archive(FE_MAKE_NVP(d()->m_name));
    archive.entities();

    for (auto& sys : d()->m_systems) sys->load(*this, archive);
  } else {
    cereal::BinaryInputArchive concreteAr(is);
    Archive archive(&concreteAr, reg);

    archive(FE_MAKE_NVP(d()->m_name));
    archive.entities();

    for (auto& sys : d()->m_systems) sys->load(*this, archive);
  }

  FE_DEBUGPRINT("Scene loaded from: %s", path.c_str());
}

void Scene::Init() {}
void Scene::PreStartup() {}
void Scene::Startup() {}
void Scene::PostStartup() {}

void Scene::First() {}
void Scene::PreUpdate() {}
void Scene::Update() {}
void Scene::PostUpdate() {}
void Scene::Last() {}

void Scene::Cleanup() {
  // 执行由各模块注入 Blackboard 的清理回调 (如 Event 交换机制)
  if (auto* cleanup_reg = m_blackboard.try_get<CleanupRegistry>()) {
    cleanup_reg->execute(m_blackboard);
  }
}

}  // namespace fe::engine