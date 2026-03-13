#include "scene.h"
#include "serialization.h"
#include "foundation/utility/assert.h"
#include <filesystem>

namespace fe::engine {

static bool is_json_path(const stl::string& path) {
    return path.size() >= 5 && path.compare(path.size() - 5, 5, ".json") == 0;
}

Scene::Scene() {
    // 内置生命周期 Pass
    m_passes.push_back(Pass::create_start<stage::Init>("Scene_Init", [](Blackboard&) {}, Priority::First));
    m_passes.push_back(Pass::create_start<stage::PreStartup>("Scene_PreStartup", [](Blackboard&) {}, Priority::First));
    m_passes.push_back(Pass::create_start<stage::Startup>("Scene_Startup", [](Blackboard&) {}, Priority::First));
    m_passes.push_back(Pass::create_start<stage::PostStartup>("Scene_PostStartup", [](Blackboard&) {}, Priority::First));

    m_passes.push_back(Pass::create_update<stage::First>("Scene_First", [](Blackboard&) {}, Priority::First));
    m_passes.push_back(Pass::create_update<stage::PreUpdate>("Scene_PreUpdate", [](Blackboard&) {}, Priority::First));
    m_passes.push_back(Pass::create_update<stage::Update>("Scene_Update", [](Blackboard&) {}, Priority::First));
    m_passes.push_back(Pass::create_update<stage::PostUpdate>("Scene_PostUpdate", [](Blackboard&) {}, Priority::First));
    m_passes.push_back(Pass::create_update<stage::Last>("Scene_Last", [](Blackboard&) {}, Priority::First));
    m_passes.push_back(Pass::create_update<stage::Cleanup>("Scene_Cleanup", [this](Blackboard& bb) {
        apply_command_buffers();
        bb.execute_cleanups();
    }, Priority::First));
}

void Scene::add_system(stl::shared_ptr<System> sys) {
    m_systems.push_back(std::move(sys));
}

void Scene::compile() {
    // 收集所有 Pass 并去重
    stl::unordered_set<stl::string> pass_names;
    stl::vector<Pass*> setup_passes, run_passes;

    for (auto& p : m_passes) {
        pass_names.insert(p.m_name);
        (p.m_repeat ? run_passes : setup_passes).push_back(&p);
    }

    for (auto& sys : m_systems) {
        if (sys->init(m_blackboard)) {
            for (auto& p : sys->m_passes) {
                if (pass_names.insert(p.m_name).second) {
                    (p.m_repeat ? run_passes : setup_passes).push_back(&p);
                }
            }
        }
    }

    // 准备所有 Pass（调用 prepare）
    for (auto* p : setup_passes) p->prepare(m_blackboard);
    for (auto* p : run_passes)   p->prepare(m_blackboard);

    // 清空旧任务流
    m_setup_tf.clear();
    m_run_tf.clear();

    if (setup_passes.empty() && run_passes.empty()) return;

    // 构建阶段屏障
    auto setup_barriers = build_stage_barriers(m_setup_tf, setup_passes);
    auto run_barriers   = build_stage_barriers(m_run_tf, run_passes);

    // 创建 Pass 任务
    stl::map<Pass*, tf::Task> setup_tasks, run_tasks;
    for (auto* p : setup_passes)
        setup_tasks[p] = m_setup_tf.emplace([p, this] { p->execute(m_blackboard); }).name(p->m_name.c_str());
    for (auto* p : run_passes)
        run_tasks[p]   = m_run_tf.emplace([p, this] { p->execute(m_blackboard); }).name(p->m_name.c_str());

    // 链接 Pass 到阶段屏障
    auto link = [&](auto& tasks, auto& passes, auto& barriers) {
        for (auto* p : passes) {
            auto it = barriers.find(p->m_stage);
            if (it != barriers.end()) {
                it->second.first.precede(tasks[p]);
                it->second.second.succeed(tasks[p]);
            }
        }
    };
    link(setup_tasks, setup_passes, setup_barriers);
    link(run_tasks,   run_passes,   run_barriers);

    // 构建冲突边
    auto build_conflict_edges = [&](auto& tasks, auto& passes) {
        size_t n = passes.size();
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = i + 1; j < n; ++j) {
                auto* a = passes[i];
                auto* b = passes[j];
                if (a->is_conflict(*b)) {
                    bool a_before_b = is_stage_reachable(a->m_stage, b->m_stage);
                    bool b_before_a = is_stage_reachable(b->m_stage, a->m_stage);
                    if (!a_before_b && !b_before_a) {
                        // 阶段不可达，按优先级排序
                        if (a->m_priority <= b->m_priority)
                            tasks[a].precede(tasks[b]);
                        else
                            tasks[b].precede(tasks[a]);
                    }
                }
            }
        }
    };
    build_conflict_edges(setup_tasks, setup_passes);
    build_conflict_edges(run_tasks,   run_passes);
}

void Scene::setup() {
    try {
        m_executor.run(m_setup_tf).wait();
    } catch (const std::exception& e) {
        FE_ERROR("Setup failed: %s", e.what());
        throw;
    }
}

void Scene::run() {
    try {
        m_executor.run(m_run_tf).wait();
    } catch (const std::exception& e) {
        FE_ERROR("Run failed: %s", e.what());
        throw;
    }
}

void Scene::dump_graph(const stl::string& path) {
    std::filesystem::path base(path.c_str());
    if (base.has_parent_path())
        std::filesystem::create_directories(base.parent_path());

    auto dir = base.parent_path();
    auto stem = base.stem().string();
    {
        std::ofstream ofs(dir / (stem + "_setup.dot"));
        if (ofs) m_setup_tf.dump(ofs);
    }
    {
        std::ofstream ofs(dir / (stem + "_run.dot"));
        if (ofs) m_run_tf.dump(ofs);
    }
}

void Scene::save(const stl::string& path) {
    std::ofstream os(path.c_str(), std::ios::binary);
    if (!os) {
        FE_ERROR("Failed to open file for saving: %s", path.c_str());
        return;
    }

    auto& reg = m_blackboard.get_or_emplace<Registry>();
    if (is_json_path(path)) {
        cereal::JSONOutputArchive ar(os);
        Archive archive(&ar, &reg);
        archive.entities();
        for (auto& sys : m_systems) sys->save(m_blackboard, archive);
    } else {
        cereal::BinaryOutputArchive ar(os);
        Archive archive(&ar, &reg);
        archive.entities();
        for (auto& sys : m_systems) sys->save(m_blackboard, archive);
    }
    FE_DEBUGPRINT("Scene saved to: %s", path.c_str());
}

void Scene::load(const stl::string& path) {
    std::ifstream is(path.c_str(), std::ios::binary);
    if (!is) {
        FE_ERROR("Failed to open file for loading: %s", path.c_str());
        return;
    }

    m_blackboard.emplace<Registry>(Registry{});
    auto& reg = m_blackboard.get<Registry>();

    if (is_json_path(path)) {
        cereal::JSONInputArchive ar(is);
        Archive archive(&ar, &reg);
        archive.entities();
        for (auto& sys : m_systems) sys->load(m_blackboard, archive);
    } else {
        cereal::BinaryInputArchive ar(is);
        Archive archive(&ar, &reg);
        archive.entities();
        for (auto& sys : m_systems) sys->load(m_blackboard, archive);
    }
    FE_DEBUGPRINT("Scene loaded from: %s", path.c_str());
}

stl::map<stage::StageHash, stl::pair<tf::Task, tf::Task>>
Scene::build_stage_barriers(tf::Taskflow& tf, const stl::vector<Pass*>& passes) {
    stl::map<stage::StageHash, stl::pair<tf::Task, tf::Task>> barriers;
    stl::unordered_set<stage::StageHash> used_stages;
    stl::vector<stage::StageHash> queue;

    // 收集所有用到的阶段
    for (auto* p : passes) {
        if (used_stages.insert(p->m_stage).second)
            queue.push_back(p->m_stage);
    }

    // 扩展依赖阶段（确保所有依赖都存在）
    for (size_t i = 0; i < queue.size(); ++i) {
        auto curr = queue[i];
        for (auto next : stage::g_stage_after[curr])
            if (used_stages.insert(next).second) queue.push_back(next);
        for (auto prev : stage::g_stage_before[curr])
            if (used_stages.insert(prev).second) queue.push_back(prev);
    }

    // 为每个阶段创建一对屏障任务（开始和结束）
    for (auto hash : used_stages) {
        auto name = stage::g_stage_names.count(hash) ? stage::g_stage_names[hash] : "Unknown";
        auto& p = barriers[hash];
        p.first = tf.emplace([]{}).name((name + "_Begin").c_str());
        p.second = tf.emplace([]{}).name((name + "_End").c_str());
        p.first.precede(p.second);
    }

    // 根据阶段依赖链接屏障
    for (auto hash : used_stages) {
        for (auto prev : stage::g_stage_before[hash]) {
            if (auto it = barriers.find(prev); it != barriers.end())
                it->second.second.precede(barriers[hash].first);
        }
        for (auto next : stage::g_stage_after[hash]) {
            if (auto it = barriers.find(next); it != barriers.end())
                it->second.first.succeed(barriers[hash].second);
        }
    }
    return barriers;
}

bool Scene::is_stage_reachable(stage::StageHash from, stage::StageHash to) const {
    if (from == to) return false;
    stl::unordered_set<stage::StageHash> visited{from};
    stl::vector<stage::StageHash> queue{from};
    for (size_t i = 0; i < queue.size(); ++i) {
        auto curr = queue[i];
        if (curr == to) return true;
        for (auto next : stage::g_stage_after[curr])
            if (visited.insert(next).second) queue.push_back(next);
    }
    return false;
}

void Scene::apply_command_buffers() {
    if (m_blackboard.has<EntityCommandBuffer>()) {
        auto& ecb = m_blackboard.get<EntityCommandBuffer>();
        auto& reg = m_blackboard.get_or_emplace<Registry>();
        for (auto& e : ecb.m_entity_map) {
            if (e == entt::null) e = reg.create();
        }
        for (auto e : ecb.m_destroyed_entities) {
            reg.destroy(e);
        }
        ecb.m_destroyed_entities.clear();
        ecb.m_entity_map.clear();
    }
}

} // namespace fe::engine
