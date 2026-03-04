#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <memory>
#include <vector>
#include <random>
#include <iomanip>

#include "engine/ecs/world.h"
#include "engine/ecs/system.h"
#include "engine/ecs/pass.h"
#include "engine/ecs/stage.h"
#include "engine/ecs/accessComponent.h"
#include "engine/ecs/accessEntity.h"
#include "engine/ecs/accessResource.h"
#include "engine/ecs/accessEvent.h"

using namespace fe::engine::ecs;
using namespace fe::engine;

// ==========================================
// 1. 定义数据层：组件、资源、事件
// ==========================================

struct Position {
  float x, y, z;
};
struct Velocity {
  float vx, vy, vz;
};
struct Health {
  float current;
  float max;
};
struct Lifetime {
  int ticks;
};

// 模拟各种独立且耗时的计算组件 (用于最大化并行)
struct ComputeDataA {
  float matrix[16];
};
struct ComputeDataB {
  float distanceMap[16];
};
struct ComputeDataC {
  float colorGrad[16];
};
struct ComputeDataD {
  float physicsSim[16];
};

// 资源：全局时钟与统计
struct GlobalTime {
  float dt;
  uint64_t frameCount;
};

struct GameStats {
  uint32_t activeEntities;
  uint32_t totalSpawned;
  uint32_t totalDestroyed;
};

// 事件：伤害事件
struct DamageEvent {
  entt::entity target;
  float amount;
};

// ==========================================
// 2. 定义逻辑层：系统 (Systems)
// ==========================================

// 初始化系统：设定初始资源
class InitSystem : public System {
 public:
  InitSystem() : System("InitSystem") {}

  bool init() override {
    m_passes.push_back(
        Pass::create_start<stage::Init>("InitResources", [](ResourceWriter<GlobalTime> timeWriter, ResourceWriter<GameStats> statsWriter) {
          timeWriter.create(GlobalTime{0.016f, 0});
          statsWriter.create(GameStats{0, 0, 0});
          std::cout << "[InitSystem] Resources initialized.\n";
        }));
    return true;
  }
};

// 生成系统：负责初始批量生成和每帧动态生成实体
class SpawnerSystem : public System {
 public:
  SpawnerSystem() : System("SpawnerSystem") {}

  static void SpawnEntities(EntityCreator& creator,
                            ComponentWriter<Position, Velocity, Health, Lifetime, ComputeDataA, ComputeDataB, ComputeDataC, ComputeDataD>& writer,
                            int count, GameStats& stats) {
    for (int i = 0; i < count; ++i) {
      auto e = creator.create();
      float offset = static_cast<float>(i);

      writer.add<Position>(e, offset, offset * 0.5f, 0.0f);
      writer.add<Velocity>(e, std::sin(offset), std::cos(offset), 0.0f);
      writer.add<Health>(e, 100.0f, 100.0f);
      // 实体存活的帧数，介于 50 到 150 帧之间
      writer.add<Lifetime>(e, 50 + (i % 100));

      writer.add<ComputeDataA>(e);
      writer.add<ComputeDataB>(e);
      writer.add<ComputeDataC>(e);
      writer.add<ComputeDataD>(e);
    }
    stats.activeEntities += count;
    stats.totalSpawned += count;
  }

  bool init() override {
    // Startup阶段：一次性生成 300,000 个实体
    m_passes.push_back(Pass::create_start<stage::Startup>(
        "InitialSpawn", [](EntityCreator creator,
                           ComponentWriter<Position, Velocity, Health, Lifetime, ComputeDataA, ComputeDataB, ComputeDataC, ComputeDataD> writer,
                           ResourceWriter<GameStats> stats) {
          SpawnEntities(creator, writer, 300000, stats.get());
          std::cout << "[SpawnerSystem] Initialized 300,000 entities.\n";
        }));

    // PreUpdate阶段：每帧动态生成 5,000 个新实体 (模拟弹幕/粒子)
    m_passes.push_back(Pass::create_update<stage::PreUpdate>(
        "DynamicSpawn", [](EntityCreator creator,
                           ComponentWriter<Position, Velocity, Health, Lifetime, ComputeDataA, ComputeDataB, ComputeDataC, ComputeDataD> writer,
                           ResourceWriter<GameStats> stats) { SpawnEntities(creator, writer, 5000, stats.get()); }));
    return true;
  }
};

// 核心移动系统
class MovementSystem : public System {
 public:
  MovementSystem() : System("MovementSystem") {}

  bool init() override {
    // 互斥锁逻辑: 读 Velocity，写 Position。
    m_passes.push_back(Pass::create_update<stage::Update>(
        "UpdateMovement", [](ComponentWriter<Position> posWriter, ComponentReader<Velocity> velReader, ResourceReader<GlobalTime> timeRes) {
          float dt = timeRes.get().dt;
          for (auto e : posWriter.view()) {
            if (velReader.have<Velocity>(e)) {
              auto& pos = posWriter.get<Position>(e);
              const auto& vel = velReader.get<Velocity>(e);
              pos.x += vel.vx * dt;
              pos.y += vel.vy * dt;
              pos.z += vel.vz * dt;
            }
          }
        }));
    return true;
  }
};

// --- 下面四个 Compute System 完美展示 Taskflow 的并行能力 ---
// 它们都只读 Position/Velocity，但分别写入独立的 A/B/C/D 组件，无互斥冲突。

class ParallelComputeSystem : public System {
 public:
  ParallelComputeSystem() : System("ParallelComputeSystem") {}

  bool init() override {
    // Compute A (模拟矩阵变换)
    m_passes.push_back(
        Pass::create_update<stage::Update>("ComputeA", [](ComponentWriter<ComputeDataA> compWriter, ComponentReader<Position, Velocity> reader) {
          for (auto e : compWriter.view()) {
            if (reader.have_all<Position, Velocity>(e)) {
              auto& data = compWriter.get<ComputeDataA>(e);
              const auto& pos = reader.get<Position>(e);
              // 插入耗时计算
              for (int i = 0; i < 16; ++i) {
                data.matrix[i] = std::sin(pos.x * i) + std::cos(pos.y * i);
              }
            }
          }
        }));

    // Compute B (模拟光照剔除计算)
    m_passes.push_back(
        Pass::create_update<stage::Update>("ComputeB", [](ComponentWriter<ComputeDataB> compWriter, ComponentReader<Position, Velocity> reader) {
          for (auto e : compWriter.view()) {
            if (reader.have_all<Position, Velocity>(e)) {
              auto& data = compWriter.get<ComputeDataB>(e);
              const auto& vel = reader.get<Velocity>(e);
              for (int i = 0; i < 16; ++i) {
                data.distanceMap[i] = std::sqrt(std::abs(vel.vx * i + vel.vy));
              }
            }
          }
        }));

    // Compute C (模拟布料/骨骼动画)
    m_passes.push_back(
        Pass::create_update<stage::Update>("ComputeC", [](ComponentWriter<ComputeDataC> compWriter, ComponentReader<Position, Velocity> reader) {
          for (auto e : compWriter.view()) {
            if (reader.have_all<Position, Velocity>(e)) {
              auto& data = compWriter.get<ComputeDataC>(e);
              const auto& pos = reader.get<Position>(e);
              for (int i = 0; i < 16; ++i) {
                data.colorGrad[i] = std::tan(pos.x + i * 0.01f);
              }
            }
          }
        }));

    // Compute D (模拟AI感知逻辑)
    m_passes.push_back(
        Pass::create_update<stage::Update>("ComputeD", [](ComponentWriter<ComputeDataD> compWriter, ComponentReader<Position, Velocity> reader) {
          for (auto e : compWriter.view()) {
            if (reader.have_all<Position, Velocity>(e)) {
              auto& data = compWriter.get<ComputeDataD>(e);
              const auto& pos = reader.get<Position>(e);
              for (int i = 0; i < 16; ++i) {
                data.physicsSim[i] = std::exp(-std::abs(pos.y * 0.001f));
              }
            }
          }
        }));
    return true;
  }
};

// 战斗系统：随机对实体产生伤害事件
class CombatSystem : public System {
 public:
  CombatSystem() : System("CombatSystem") {}

  bool init() override {
    // 发射伤害事件 (PostUpdate 阶段)
    m_passes.push_back(Pass::create_update<stage::PostUpdate>(
        "EmitDamageEvents", [](EventWriter<DamageEvent> eventWriter, ComponentReader<Health> healthReader, ResourceReader<GlobalTime> timeRes) {
          int frame = timeRes.get().frameCount;
          int count = 0;
          // 模拟每帧对小部分实体产生伤害
          for (auto e : healthReader.view()) {
            if (static_cast<uint32_t>(e) % 100 == (frame % 100)) {
              eventWriter.get().push_back({e, 25.0f});
              if (++count > 2000)
                break;  // 限制最多2000个事件
            }
          }
        }));

    // 处理伤害事件 (PostUpdate 阶段，需在 Emit 之后，框架的任务流会自动处理同阶段基于资源的依赖)
    m_passes.push_back(
        Pass::create_update<stage::PostUpdate>("ProcessDamage", [](EventReader<DamageEvent> eventReader, ComponentWriter<Health> healthWriter) {
          for (const auto& ev : eventReader.get()) {
            if (healthWriter.have<Health>(ev.target)) {
              auto& hp = healthWriter.get<Health>(ev.target);
              hp.current -= ev.amount;
            }
          }
        }));
    return true;
  }
};

// 生命周期系统：扣减寿命并清理死亡实体
class LifeCycleSystem : public System {
 public:
  LifeCycleSystem() : System("LifeCycleSystem") {}

  bool init() override {
    // Last 阶段执行，具有最高互斥级 EntityDestroyer
    m_passes.push_back(
        Pass::create_update<stage::Last>("DeathAndCleanup", [](EntityDestroyer destroyer, ComponentWriter<Lifetime> lifeWriter,
                                                               ComponentReader<Health> healthReader, ResourceWriter<GameStats> statsWriter) {
          int destroyedThisFrame = 0;
          auto view = lifeWriter.view();

          // 为了安全销毁，先收集要销毁的实体（避免在迭代 EnTT view 时破坏结构）
          std::vector<entt::entity> toDestroy;
          toDestroy.reserve(10000);

          for (auto e : view) {
            auto& life = lifeWriter.get<Lifetime>(e);
            life.ticks--;

            bool dead = false;
            if (life.ticks <= 0) {
              dead = true;
            } else if (healthReader.have<Health>(e)) {
              if (healthReader.get<Health>(e).current <= 0.0f) {
                dead = true;
              }
            }

            if (dead) {
              toDestroy.push_back(e);
            }
          }

          // 批量销毁
          for (auto e : toDestroy) {
            destroyer.destroy(e);
            destroyedThisFrame++;
          }

          auto& stats = statsWriter.get();
          stats.totalDestroyed += destroyedThisFrame;
          stats.activeEntities -= destroyedThisFrame;
        }));
    return true;
  }
};

// 统计与监控系统
class MonitorSystem : public System {
 public:
  MonitorSystem() : System("MonitorSystem") {}

  bool init() override {
    m_passes.push_back(
        Pass::create_update<stage::Cleanup>("TickAndLog", [](ResourceWriter<GlobalTime> timeWriter, ResourceReader<GameStats> statsReader) {
          auto& time = timeWriter.get();
          time.frameCount++;
        }));
    return true;
  }
};

// ==========================================
// 3. 性能测试入口主程序
// ==========================================

int main() {
  std::cout << "===========================================\n";
  std::cout << " FantasyEngine ECS - High Performance Test\n";
  std::cout << "===========================================\n";

  try {
    World world;

    // 注册所有系统
    world.add_system(stl::make_shared<InitSystem>());
    world.add_system(stl::make_shared<SpawnerSystem>());
    world.add_system(stl::make_shared<MovementSystem>());
    world.add_system(stl::make_shared<ParallelComputeSystem>());
    world.add_system(stl::make_shared<CombatSystem>());
    world.add_system(stl::make_shared<LifeCycleSystem>());
    world.add_system(stl::make_shared<MonitorSystem>());

    // 编译任务图 (Taskflow 自动通过互斥锁推导执行顺序与并发层级)
    std::cout << "\n[1] Compiling execution graph...\n";
    auto compileStart = std::chrono::high_resolution_clock::now();
    world.compile();
    auto compileEnd = std::chrono::high_resolution_clock::now();
    std::cout << "    -> Compiled in " << std::chrono::duration_cast<std::chrono::milliseconds>(compileEnd - compileStart).count() << " ms.\n";

    // 输出任务流图纸，可在 Graphviz (dot) 中查看其精美的并行结构
    world.dump_graph("performance_test_graph");
    std::cout << "    -> Dumped graph to performance_test_graph_run.dot\n";

    // 执行 Setup 阶段 (如 InitialSpawn 等)
    std::cout << "\n[2] Running Setup Phase...\n";
    world.setup();

    // 核心 Benchmark 循环
    const int TEST_FRAMES = 500;
    std::cout << "\n[3] Running Benchmark for " << TEST_FRAMES << " frames...\n\n";

    std::vector<double> frameTimes;
    frameTimes.reserve(TEST_FRAMES);

    auto testStartTime = std::chrono::high_resolution_clock::now();

    for (int frame = 0; frame < TEST_FRAMES; ++frame) {
      auto frameStart = std::chrono::high_resolution_clock::now();

      world.run();  // 执行一帧

      auto frameEnd = std::chrono::high_resolution_clock::now();
      double frameTimeMs = std::chrono::duration<double, std::milli>(frameEnd - frameStart).count();
      frameTimes.push_back(frameTimeMs);

      // 每 50 帧打印一次状态监控
      if ((frame + 1) % 50 == 0) {
        // 通过 World 的系统机制临时拿取全局统计信息 (直接取有些违背ECS的封装，这里作为测试用特例)
        // 为简便起见，这里仅输出当前的性能状态。
        std::cout << "    [Frame " << std::setw(3) << (frame + 1) << "] "
                  << "Time: " << std::fixed << std::setprecision(2) << frameTimeMs << " ms "
                  << "(" << std::setw(5) << static_cast<int>(1000.0 / frameTimeMs) << " FPS)\n";
      }
    }

    auto testEndTime = std::chrono::high_resolution_clock::now();
    double totalTestTime = std::chrono::duration<double>(testEndTime - testStartTime).count();

    // 分析 Benchmark 结果
    std::cout << "\n===========================================\n";
    std::cout << "             BENCHMARK RESULTS             \n";
    std::cout << "===========================================\n";

    double minTime = frameTimes[0], maxTime = frameTimes[0], sumTime = 0.0;
    for (double t : frameTimes) {
      if (t < minTime)
        minTime = t;
      if (t > maxTime)
        maxTime = t;
      sumTime += t;
    }
    double avgTime = sumTime / TEST_FRAMES;

    std::cout << " Total Frames     : " << TEST_FRAMES << "\n";
    std::cout << " Total Time (s)   : " << std::fixed << std::setprecision(3) << totalTestTime << " s\n";
    std::cout << " Max Frame Time   : " << maxTime << " ms\n";
    std::cout << " Min Frame Time   : " << minTime << " ms\n";
    std::cout << " Avg Frame Time   : " << avgTime << " ms\n";
    std::cout << " Avg FPS          : " << (1000.0 / avgTime) << "\n";
    std::cout << "===========================================\n";

  } catch (const std::exception& e) {
    std::cerr << "\n[FATAL ERROR] " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}