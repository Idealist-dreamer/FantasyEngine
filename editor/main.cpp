#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <memory>
#include <vector>
#include <iomanip>
#include <atomic>
#include <random>

#include "engine/ecs/world.h"
#include "engine/ecs/system.h"
#include "engine/ecs/pass.h"
#include "engine/ecs/stage.h"
#include "engine/ecs/paramTypes.h"

using namespace fe::engine::ecs;
using namespace fe::engine;

// ==========================================
// 权威验证级全局探针 (绝对防止编译器优化)
// ==========================================
inline std::atomic<uint64_t> g_setup_executions{0};
inline std::atomic<uint64_t> g_total_loop_iterations{0};
inline std::atomic<uint64_t> g_global_state_hash{0};

// ==========================================
// 1. 数据层定义
// ==========================================
struct Transform {
  float pos[3];
  float rot[3];
};
struct Velocity {
  float v[3];
};
struct BoidState {
  float target[3];
  float speedMultiplier;
};
struct RenderState {
  float matrix[16];
};
struct ParticleData {
  float pos[3];
  float vel[3];
  float life;
};
struct Health {
  float current;
};
struct GlobalTime {
  float dt;
  uint32_t frameCount;
};
struct GameStats {
  uint32_t activeEntities;
  uint32_t totalSpawned;
};

// ==========================================
// 2. 逻辑层定义
// ==========================================

class CoreInitSystem : public System {
 public:
  CoreInitSystem() : System("CoreInitSystem") {}
  bool init() override {
    m_passes.push_back(Pass::create_start<stage::Init>("InitContexts", [](ContextWriter<GlobalTime> time, ContextWriter<GameStats> stats) {
      if (!time.valid())
        time.create(GlobalTime{0.016f, 0});
      if (!stats.valid())
        stats.create(GameStats{0, 0});
    }));
    return true;
  }
};

class SpawnerSystem : public System {
 public:
  SpawnerSystem() : System("SpawnerSystem") {}
  bool init() override {
    m_passes.push_back(Pass::create_start<stage::Startup>(
        "Initial_Mass_Spawn", [](EntityCreator creator, ComponentWriter<Transform, Velocity, BoidState, RenderState, Health, ParticleData> writer,
                                 ContextWriter<GameStats> stats) {
          g_setup_executions.fetch_add(1, std::memory_order_relaxed);
          if (!stats.valid())
            stats.create(GameStats{0, 0});

          const int AGENT_COUNT = 150000;
          const int PARTICLE_COUNT = 300000;

          // 引入伪随机，打破数据规律，防止 SIMD 极简优化
          std::mt19937 rng(42);
          std::uniform_real_distribution<float> dist(-10.0f, 10.0f);

          for (int i = 0; i < AGENT_COUNT; ++i) {
            auto e = creator.create();
            writer.add<Transform>(e, Transform{{dist(rng), dist(rng), dist(rng)}, {dist(rng), 0.0f, 0.0f}});
            writer.add<Velocity>(e, Velocity{{1.0f, 0.5f, 0.1f}});
            writer.add<BoidState>(e, BoidState{{dist(rng), dist(rng), dist(rng)}, 1.0f});
            writer.add<RenderState>(e);
            writer.add<Health>(e, Health{100.0f});
          }

          for (int i = 0; i < PARTICLE_COUNT; ++i) {
            auto e = creator.create();
            writer.add<ParticleData>(e, ParticleData{{dist(rng), dist(rng), dist(rng)}, {0.0f, 1.0f, 0.0f}, 100.0f});
          }

          stats.get().activeEntities += (AGENT_COUNT + PARTICLE_COUNT);
          stats.get().totalSpawned += (AGENT_COUNT + PARTICLE_COUNT);
        }));
    return true;
  }
};

class AISystem : public System {
 public:
  AISystem() : System("AISystem") {}
  bool init() override {
    m_passes.push_back(
        Pass::create_update<stage::PreUpdate>("AI_BoidSteer", [](ComponentReader<Transform> r_trans, ComponentWriter<Velocity, BoidState> w_velboid) {
          uint64_t iters = 0;
          for (auto e : w_velboid.view<Velocity, BoidState>()) {
            if (const auto* t = r_trans.try_get<Transform>(e)) {
              auto& v = w_velboid.get<Velocity>(e);
              auto& b = w_velboid.get<BoidState>(e);

              float dx = b.target[0] - t->pos[0];
              float dy = b.target[1] - t->pos[1];
              float dz = b.target[2] - t->pos[2];
              float dist = std::sqrt(dx * dx + dy * dy + dz * dz) + 0.001f;

              v.v[0] = (dx / dist) * std::sin(t->pos[0]) * b.speedMultiplier;
              v.v[1] = (dy / dist) * std::cos(t->pos[1]) * b.speedMultiplier;
              v.v[2] = std::tan(t->rot[2]);

              b.speedMultiplier += 0.00001f;
              iters++;
            }
          }
          g_total_loop_iterations.fetch_add(iters, std::memory_order_relaxed);
        }));
    return true;
  }
};

class PhysicsSystem : public System {
 public:
  PhysicsSystem() : System("PhysicsSystem") {}
  bool init() override {
    m_passes.push_back(Pass::create_update<stage::Update>(
        "Phys_UpdateAgents", [](ComponentWriter<Transform> w_trans, ComponentReader<Velocity> r_vel, ContextReader<GlobalTime> time) {
          float dt = time.valid() ? time.get().dt : 0.016f;
          uint64_t iters = 0;
          for (auto e : w_trans.view<Transform>()) {
            if (const auto* v = r_vel.try_get<Velocity>(e)) {
              auto& t = w_trans.get<Transform>(e);
              t.pos[0] += v->v[0] * dt;
              t.pos[1] += v->v[1] * dt;
              t.pos[2] += v->v[2] * dt;
              t.rot[0] = std::fmod(t.rot[0] + dt * 0.5f, 3.14159f);
              iters++;
            }
          }
          g_total_loop_iterations.fetch_add(iters, std::memory_order_relaxed);
        }));

    m_passes.push_back(
        Pass::create_update<stage::Update>("Phys_UpdateParticles", [](ComponentWriter<ParticleData> w_part, ContextReader<GlobalTime> time) {
          float dt = time.valid() ? time.get().dt : 0.016f;
          uint64_t iters = 0;
          for (auto e : w_part.view<ParticleData>()) {
            auto& p = w_part.get<ParticleData>(e);
            float noise = std::sin(p.pos[0] * 0.1f);
            p.pos[0] += (p.vel[0] + noise) * dt;
            p.life -= dt;
            iters++;
          }
          g_total_loop_iterations.fetch_add(iters, std::memory_order_relaxed);
        }));
    return true;
  }
};

class RenderComputeSystem : public System {
 public:
  RenderComputeSystem() : System("RenderComputeSystem") {}
  bool init() override {
    m_passes.push_back(
        Pass::create_update<stage::PostUpdate>("Render_ComputeMatrix", [](ComponentReader<Transform> r_trans, ComponentWriter<RenderState> w_rend) {
          uint64_t iters = 0;
          uint64_t math_accumulator = 0;
          for (auto e : w_rend.view<RenderState>()) {
            if (const auto* t = r_trans.try_get<Transform>(e)) {
              auto& r = w_rend.get<RenderState>(e);

              float sx = std::sin(t->rot[0]);
              float cx = std::cos(t->rot[0]);
              float sy = std::sin(t->rot[1]);
              float cy = std::cos(t->rot[1]);

              r.matrix[0] = cy * cx;
              r.matrix[5] = sx * sy + cx;
              r.matrix[12] = t->pos[0];
              r.matrix[15] = 1.0f;

              // 将算力注入整数累计，绝对阻止浮点代码被剔除
              math_accumulator += static_cast<uint64_t>(std::abs(r.matrix[0] * 1000.0f));
              iters++;
            }
          }
          g_total_loop_iterations.fetch_add(iters, std::memory_order_relaxed);
          g_global_state_hash.fetch_add(math_accumulator, std::memory_order_relaxed);
        }));
    return true;
  }
};

class BenchmarkMonitorSystem : public System {
 public:
  BenchmarkMonitorSystem() : System("BenchmarkMonitorSystem") {}
  bool init() override {
    m_passes.push_back(
        Pass::create_update<stage::Cleanup>("Checksum_Eval", [](ComponentReader<RenderState, ParticleData> reader, ContextWriter<GlobalTime> time) {
          if (!time.valid())
            time.create(GlobalTime{0.016f, 0});

          // 使用 EnTT 自带的 size_hint 进行实体存在性硬校验
          uint64_t agents = reader.view<RenderState>().size();
          uint64_t particles = reader.view<ParticleData>().size();

          // 如果某帧突然为空，强制引发变化
          if (agents == 0 && particles == 0) {
            g_global_state_hash.fetch_add(999999999, std::memory_order_relaxed);
          }

          time.get().frameCount++;
        }));
    return true;
  }
};

// ==========================================
// 3. 严格受控测试启动点
// ==========================================
int main() {
  std::cout << "===========================================\n";
  std::cout << " FantasyEngine ECS - Ironclad Benchmark  \n";
  std::cout << "===========================================\n";

  try {
    World world;

    world.add_system(stl::make_shared<CoreInitSystem>());
    world.add_system(stl::make_shared<SpawnerSystem>());
    world.add_system(stl::make_shared<AISystem>());
    world.add_system(stl::make_shared<PhysicsSystem>());
    world.add_system(stl::make_shared<RenderComputeSystem>());
    world.add_system(stl::make_shared<BenchmarkMonitorSystem>());

    std::cout << "[1] Compiling execution graph...\n";
    world.compile();
    world.dump_graph("./");

    std::cout << "[2] Running Setup Phase...\n";
    world.setup();

    // 关键断言！如果此处等于 0，证明 Taskflow 在执行 Setup 阶段时失效！
    if (g_setup_executions.load() == 0) {
      std::cerr << ">>> [FATAL] Setup Phase NEVER EXECUTED! The Registry is empty!\n";
      std::cerr << ">>> Please check Taskflow dispatch logic for Setup passes.\n";
      return EXIT_FAILURE;
    }

    const int TEST_FRAMES = 500;
    std::cout << "[3] Running Benchmark for " << TEST_FRAMES << " frames...\n\n";

    std::vector<double> frameTimes;
    frameTimes.reserve(TEST_FRAMES);

    auto testStartTime = std::chrono::high_resolution_clock::now();

    for (int frame = 0; frame < TEST_FRAMES; ++frame) {
      auto frameStart = std::chrono::high_resolution_clock::now();

      world.run();

      auto frameEnd = std::chrono::high_resolution_clock::now();
      double frameTimeMs = std::chrono::duration<double, std::milli>(frameEnd - frameStart).count();
      frameTimes.push_back(frameTimeMs);

      if ((frame + 1) % 100 == 0) {
        std::cout << "[Frame " << std::setw(3) << (frame + 1) << "] "
                  << "Time: " << std::fixed << std::setprecision(2) << std::setw(6) << frameTimeMs << " ms "
                  << "(" << std::setw(5) << static_cast<int>(1000.0 / frameTimeMs) << " FPS)\n";
      }
    }

    auto testEndTime = std::chrono::high_resolution_clock::now();
    double totalTestTime = std::chrono::duration<double>(testEndTime - testStartTime).count();

    double minTime = frameTimes[0], maxTime = frameTimes[0], sumTime = 0.0;
    for (double t : frameTimes) {
      if (t < minTime)
        minTime = t;
      if (t > maxTime)
        maxTime = t;
      sumTime += t;
    }
    double avgTime = sumTime / TEST_FRAMES;

    std::cout << "\n===========================================\n";
    std::cout << "             IRONCLAD RESULTS              \n";
    std::cout << "===========================================\n";
    std::cout << " Total Frames     : " << TEST_FRAMES << "\n";
    std::cout << " Total Time (s)   : " << std::fixed << std::setprecision(3) << totalTestTime << " s\n";
    std::cout << " Max Frame Time   : " << maxTime << " ms\n";
    std::cout << " Min Frame Time   : " << minTime << " ms\n";
    std::cout << " Avg Frame Time   : " << avgTime << " ms\n";
    std::cout << " Avg FPS          : " << static_cast<int>(1000.0 / avgTime) << "\n";
    std::cout << "-------------------------------------------\n";
    std::cout << " Iterations Exec  : " << g_total_loop_iterations.load() << " loops\n";
    std::cout << " Operations Hash  : " << g_global_state_hash.load() << "\n";
    std::cout << "===========================================\n";

    // 如果迭代次数为 0，说明视图内根本没东西
    if (g_total_loop_iterations.load() == 0) {
      std::cerr << "\n>>> [WARNING] 0 iterations detected! The Views are EMPTY during update.\n";
      std::cerr << ">>> Reason: Either `m_reg.emplace_or_replace` failed silently, or Entity creation didn't persist.\n";
    }

  } catch (const std::exception& e) {
    std::cerr << "\n[FATAL ERROR] " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}