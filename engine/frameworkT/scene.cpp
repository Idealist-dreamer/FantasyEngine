#include "scene.h"
#include "serialization.h"
#include <filesystem>

namespace fe::engine {

// Helper to check if path is JSON
static bool is_json_path(const stl::string& path) {
    return path.size() >= 5 && path.compare(path.size() - 5, 5, ".json") == 0;
}

Scene::Scene() {
    // Default lifecycle passes
    m_passes.push_back(Pass::create_start<stage::Init>("Scene_Init", [this]() {}, Priority::First));
    m_passes.push_back(Pass::create_start<stage::PreStartup>("Scene_PreStartup", [this]() {}, Priority::First));
    m_passes.push_back(Pass::create_start<stage::Startup>("Scene_Startup", [this]() {}, Priority::First));
    m_passes.push_back(Pass::create_start<stage::PostStartup>("Scene_PostStartup", [this]() {}, Priority::First));

    m_passes.push_back(Pass::create_update<stage::First>("Scene_First", [this]() {}, Priority::First));
    m_passes.push_back(Pass::create_update<stage::PreUpdate>("Scene_PreUpdate", [this]() {}, Priority::First));
    m_passes.push_back(Pass::create_update<stage::Update>("Scene_Update", [this]() {}, Priority::First));
    m_passes.push_back(Pass::create_update<stage::PostUpdate>("Scene_PostUpdate", [this]() {}, Priority::First));
    m_passes.push_back(Pass::create_update<stage::Last>("Scene_Last", [this]() {}, Priority::First));
    m_passes.push_back(Pass::create_update<stage::Cleanup>("Scene_Cleanup", [this]() {
        apply_command_buffers();
        m_blackboard.execute_cleanups();
        m_blackboard.clear_cleanups();
    }, Priority::First));
}

void Scene::add_system(stl::shared_ptr<System> sys) {
    m_systems.push_back(std::move(sys));
}

void Scene::compile() {
    // Collect passes
    stl::unordered_set<stl::string> pass_names;
    stl::vector<Pass*> setup_passes;
    stl::vector<Pass*> run_passes;

    for (auto& p : m_passes) {
        pass_names.insert(p.m_name);
        if (p.m_repeat) run_passes.push_back(&p);
        else setup_passes.push_back(&p);
    }

    // Add system passes
    for (auto& sys : m_systems) {
        if (sys->init(m_blackboard)) {
            for (auto& p : sys->m_passes) {
                if (pass_names.insert(p.m_name).second) {
                    if (p.m_repeat) run_passes.push_back(&p);
                    else setup_passes.push_back(&p);
                }
            }
        }
    }

    // Prepare all passes
    for (auto* p : setup_passes) p->prepare(m_blackboard);
    for (auto* p : run_passes) p->prepare(m_blackboard);

    // Build taskflows
    m_setup_tf.clear();
    m_run_tf.clear();

    if (setup_passes.empty() && run_passes.empty()) return;

    // Build stage barriers
    auto setup_barriers = build_stage_barriers(m_setup_tf, setup_passes);
    auto run_barriers = build_stage_barriers(m_run_tf, run_passes);

    // Create pass tasks
    stl::map<Pass*, tf::Task> setup_tasks;
    stl::map<Pass*, tf::Task> run_tasks;

    for (auto* p : setup_passes) {
        setup_tasks[p] = m_setup_tf.emplace([p, this]() { p->execute(m_blackboard); })
            .name(p->m_name.c_str());
    }
    for (auto* p : run_passes) {
        run_tasks[p] = m_run_tf.emplace([p, this]() { p->execute(m_blackboard); })
            .name(p->m_name.c_str());
    }

    // Link passes to stage barriers
    for (auto* p : setup_passes) {
        auto it = setup_barriers.find(p->m_stage);
        if (it != setup_barriers.end()) {
            it->second.first.precede(setup_tasks[p]);
            it->second.second.succeed(setup_tasks[p]);
        }
    }
    for (auto* p : run_passes) {
        auto it = run_barriers.find(p->m_stage);
        if (it != run_barriers.end()) {
            it->second.first.precede(run_tasks[p]);
            it->second.second.succeed(run_tasks[p]);
        }
    }

    // Build conflict edges
    auto build_conflicts = [&](stl::map<Pass*, tf::Task>& tasks, const stl::vector<Pass*>& passes) {
        size_t n = passes.size();
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = i + 1; j < n; ++j) {
                auto* a = passes[i];
                auto* b = passes[j];
                if (a->is_conflict(*b)) {
                    bool a_before_b = is_stage_reachable(a->m_stage, b->m_stage);
                    bool b_before_a = is_stage_reachable(b->m_stage, a->m_stage);
                    if (!a_before_b && !b_before_a) {
                        if (a->m_priority <= b->m_priority) {
                            tasks[a].precede(tasks[b]);
                        } else {
                            tasks[b].precede(tasks[a]);
                        }
                    }
                }
            }
        }
    };

    build_conflicts(setup_tasks, setup_passes);
    build_conflicts(run_tasks, run_passes);
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
    if (base.has_parent_path()) {
        std::filesystem::create_directories(base.parent_path());
    }

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
        for (auto& sys : m_systems) {
            sys->save(m_blackboard, archive);
        }
    } else {
        cereal::BinaryOutputArchive ar(os);
        Archive archive(&ar, &reg);
        archive.entities();
        for (auto& sys : m_systems) {
            sys->save(m_blackboard, archive);
        }
    }

    FE_DEBUGPRINT("Scene saved to: %s", path.c_str());
}

void Scene::load(const stl::string& path) {
    std::ifstream is(path.c_str(), std::ios::binary);
    if (!is) {
        FE_ERROR("Failed to open file for loading: %s", path.c_str());
        return;
    }

    // Reset registry
    m_blackboard.emplace<Registry>(Registry{});
    auto& reg = m_blackboard.get<Registry>();

    if (is_json_path(path)) {
        cereal::JSONInputArchive ar(is);
        Archive archive(&ar, &reg);
        archive.entities();
        for (auto& sys : m_systems) {
            sys->load(m_blackboard, archive);
        }
    } else {
        cereal::BinaryInputArchive ar(is);
        Archive archive(&ar, &reg);
        archive.entities();
        for (auto& sys : m_systems) {
            sys->load(m_blackboard, archive);
        }
    }

    FE_DEBUGPRINT("Scene loaded from: %s", path.c_str());
}

stl::map<stage::StageHash, stl::pair<tf::Task, tf::Task>>
Scene::build_stage_barriers(tf::Taskflow& tf, const stl::vector<Pass*>& passes) {
    stl::map<stage::StageHash, stl::pair<tf::Task, tf::Task>> barriers;
    stl::unordered_set<stage::StageHash> used_stages;
    stl::vector<stage::StageHash> queue;

    // Collect used stages
    for (auto* p : passes) {
        if (used_stages.insert(p->m_stage).second) {
            queue.push_back(p->m_stage);
        }
    }

    // Expand stage dependencies
    for (size_t head = 0; head < queue.size(); ++head) {
        auto curr = queue[head];
        for (auto next : stage::g_stage_after[curr]) {
            if (used_stages.insert(next).second) queue.push_back(next);
        }
        for (auto prev : stage::g_stage_before[curr]) {
            if (used_stages.insert(prev).second) queue.push_back(prev);
        }
    }

    // Create barrier tasks
    for (auto hash : used_stages) {
        auto name = stage::g_stage_names.count(hash) ? stage::g_stage_names[hash] : "unknown";
        barriers[hash].first = tf.emplace([](){}).name((name + "_Begin").c_str());
        barriers[hash].second = tf.emplace([](){}).name((name + "_End").c_str());
        barriers[hash].first.precede(barriers[hash].second);
    }

    // Link barriers by stage dependencies
    for (auto hash : used_stages) {
        for (auto prev : stage::g_stage_before[hash]) {
            if (barriers.count(prev)) {
                barriers[prev].second.precede(barriers[hash].first);
            }
        }
        for (auto next : stage::g_stage_after[hash]) {
            if (barriers.count(next)) {
                barriers[next].first.succeed(barriers[hash].second);
            }
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
        auto it = stage::g_stage_after.find(curr);
        if (it != stage::g_stage_after.end()) {
            for (auto next : it->second) {
                if (visited.insert(next).second) queue.push_back(next);
            }
        }
    }
    return false;
}

void Scene::apply_command_buffers() {
    // Apply entity command buffers
    if (m_blackboard.has<EntityCommandBuffer>()) {
        auto& ecb = m_blackboard.get<EntityCommandBuffer>();
        auto& reg = m_blackboard.get_or_emplace<Registry>();
        
        for (auto& e : ecb.entities) {
            if (e == entt::null) {
                e = reg.create();
            }
        }
        for (auto e : ecb.destroyed) {
            reg.destroy(e);
        }
        ecb.destroyed.clear();
    }
}

} // namespace fe::engine
