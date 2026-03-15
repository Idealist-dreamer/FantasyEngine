#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <memory>
#include <vector>
#include <iomanip>
#include <atomic>
#include <random>
#include <fstream>
#include <filesystem>

#include "engine/engine.h"

using namespace fe::engine;
using namespace fe::engine;

// ==========================================
// 全局测试压力配置 (Debug/Release 自动切换)
// ==========================================
struct TestConfig {
  // 实体数量
  int AGENT_COUNT;
  int PARTICLE_COUNT;

  // 帧数配置
  int BENCHMARK_FRAMES;      // 性能测试帧数
  int SAVELOAD_INIT_FRAMES;  // 存档测试初始帧数
  int SAVELOAD_CONT_FRAMES;  // 存档测试继续帧数

  // 抽样验证
  int MAX_SAMPLES;  // 组件数据抽样数量

  // 模式标识
  bool isDebug;

  static TestConfig create() {
    TestConfig cfg;
#ifdef _DEBUG
    cfg.isDebug = true;
    // Debug 模式：大幅降低压力，快速迭代
    cfg.AGENT_COUNT = 1000;
    cfg.PARTICLE_COUNT = 2000;
    cfg.BENCHMARK_FRAMES = 50;
    cfg.SAVELOAD_INIT_FRAMES = 10;
    cfg.SAVELOAD_CONT_FRAMES = 5;
    cfg.MAX_SAMPLES = 10;
#else
    cfg.isDebug = false;
    // Release 模式：完整压力测试
    cfg.AGENT_COUNT = 150000;
    cfg.PARTICLE_COUNT = 300000;
    cfg.BENCHMARK_FRAMES = 500;
    cfg.SAVELOAD_INIT_FRAMES = 100;
    cfg.SAVELOAD_CONT_FRAMES = 50;
    cfg.MAX_SAMPLES = 100;
#endif
    return cfg;
  }

  void print() const {
    std::cout << "Test Configuration (" << (isDebug ? "DEBUG" : "RELEASE")
              << "):\n";
    std::cout << "  Agents        : " << AGENT_COUNT << "\n";
    std::cout << "  Particles     : " << PARTICLE_COUNT << "\n";
    std::cout << "  Benchmark     : " << BENCHMARK_FRAMES << " frames\n";
    std::cout << "  SaveLoad Init : " << SAVELOAD_INIT_FRAMES << " frames\n";
    std::cout << "  SaveLoad Cont : " << SAVELOAD_CONT_FRAMES << " frames\n";
    std::cout << "  Max Samples   : " << MAX_SAMPLES << "\n";
  }
};

// 全局配置实例
inline TestConfig g_config = TestConfig::create();

// ==========================================
// 权威验证级全局探针 (绝对防止编译器优化)
// ==========================================
inline std::atomic<uint64_t> g_setup_executions{0};
inline std::atomic<uint64_t> g_total_loop_iterations{0};
inline std::atomic<uint64_t> g_global_state_hash{0};

// ==========================================
// 1. 数据层定义 (含序列化支持)
// ==========================================
struct Transform {
  float pos[3];
  float rot[3];

  template <class Archive>
  void serialize(Archive& ar) {
    ar(pos[0], pos[1], pos[2], rot[0], rot[1], rot[2]);
  }
};

struct Velocity {
  float v[3];

  template <class Archive>
  void serialize(Archive& ar) {
    ar(v[0], v[1], v[2]);
  }
};

struct BoidState {
  float target[3];
  float speedMultiplier;

  template <class Archive>
  void serialize(Archive& ar) {
    ar(target[0], target[1], target[2], speedMultiplier);
  }
};

struct RenderState {
  float matrix[16];

  template <class Archive>
  void save(Archive& ar) const {
    // 必须退化类型，去除可能附带的 & 或 const 约束，以精确匹配 Archive 原型
    using ArType = std::decay_t<Archive>;

    if constexpr (std::is_same_v<ArType, cereal::JSONOutputArchive> ||
                  std::is_same_v<ArType, cereal::XMLOutputArchive>) {
      for (int i = 0; i < 16; ++i) {
        ar(matrix[i]);
      }
    } else {
      ar(cereal::binary_data(matrix, sizeof(matrix)));
    }
  }

  template <class Archive>
  void load(Archive& ar) {
    using ArType = std::decay_t<Archive>;

    if constexpr (std::is_same_v<ArType, cereal::JSONInputArchive> ||
                  std::is_same_v<ArType, cereal::XMLInputArchive>) {
      for (int i = 0; i < 16; ++i) {
        ar(matrix[i]);
      }
    } else {
      ar(cereal::binary_data(matrix, sizeof(matrix)));
    }
  }
};

struct ParticleData {
  float pos[3];
  float vel[3];
  float life;

  template <class Archive>
  void serialize(Archive& ar) {
    ar(pos[0], pos[1], pos[2], vel[0], vel[1], vel[2], life);
  }
};

struct Health {
  float current;

  template <class Archive>
  void serialize(Archive& ar) {
    ar(current);
  }
};

struct GlobalTime {
  float dt;
  uint32_t frameCount;

  template <class Archive>
  void serialize(Archive& ar) {
    ar(dt, frameCount);
  }
};

struct GameStats {
  uint32_t activeEntities;
  uint32_t totalSpawned;

  template <class Archive>
  void serialize(Archive& ar) {
    ar(activeEntities, totalSpawned);
  }
};

// ==========================================
// 2. 逻辑层定义 (含序列化支持)
// ==========================================

class CoreInitSystem : public System {
 public:
  CoreInitSystem() : System("CoreInitSystem") {}

  bool init(SceneBase& scene) override {
    m_passes.push_back(Pass::create_start<stage::Init>(
        "InitContexts",
        [](ContextWriter<GlobalTime> time, ContextWriter<GameStats> stats) {
          if (!time.valid()) time.create(GlobalTime{0.016f, 0});
          if (!stats.valid()) stats.create(GameStats{0, 0});
        }));
    return true;
  }

  void save(SceneBase& scene, Archive& ar) override {
    auto timeIt = scene.m_context_manager.find(typeid(GlobalTime));
    auto statsIt = scene.m_context_manager.find(typeid(GameStats));

    if (timeIt != scene.m_context_manager.end() && timeIt->second.valid()) {
      auto* time = timeIt->second.get<GlobalTime>();
      if (time) {
        ar(FE_MAKE_NVP(*time));
      }
    }
    if (statsIt != scene.m_context_manager.end() && statsIt->second.valid()) {
      auto* stats = statsIt->second.get<GameStats>();
      if (stats) {
        ar(FE_MAKE_NVP(*stats));
      }
    }
  }

  void load(SceneBase& scene, Archive& ar) override {
    // 重建上下文（如果不存在或无效）
    // 注意：compile() 期间 ParamAdapter::prepare 可能已创建空的 Any 占位符
    auto timeIt = scene.m_context_manager.find(typeid(GlobalTime));
    if (timeIt == scene.m_context_manager.end() || !timeIt->second.valid()) {
      scene.m_context_manager[typeid(GlobalTime)] =
          Any::create<GlobalTime>(0.016f, 0);
    }

    auto statsIt = scene.m_context_manager.find(typeid(GameStats));
    if (statsIt == scene.m_context_manager.end() || !statsIt->second.valid()) {
      scene.m_context_manager[typeid(GameStats)] = Any::create<GameStats>(0, 0);
    }

    // 安全获取上下文指针
    auto& timeAny = scene.m_context_manager.at(typeid(GlobalTime));
    auto& statsAny = scene.m_context_manager.at(typeid(GameStats));

    if (!timeAny.valid() || !statsAny.valid()) {
      std::cerr << "[CoreInitSystem::load] Context creation failed!\n";
      return;
    }

    auto* time = timeAny.get<GlobalTime>();
    auto* stats = statsAny.get<GameStats>();

    if (time && stats) {
      ar(FE_MAKE_NVP(*time));
      ar(FE_MAKE_NVP(*stats));
    }
  }
};

class SpawnerSystem : public System {
 public:
  SpawnerSystem() : System("SpawnerSystem") {}

  bool init(SceneBase& scene) override {
    m_passes.push_back(Pass::create_start<stage::Startup>(
        "Initial_Mass_Spawn",
        [](EntityCreator creator,
           ComponentWriter<Transform, Velocity, BoidState, RenderState, Health,
                           ParticleData>
               writer,
           ContextWriter<GameStats> stats) {
          g_setup_executions.fetch_add(1, std::memory_order_relaxed);
          if (!stats.valid()) stats.create(GameStats{0, 0});

          // 使用全局配置
          const int AGENT_COUNT = g_config.AGENT_COUNT;
          const int PARTICLE_COUNT = g_config.PARTICLE_COUNT;

          // 引入伪随机，打破数据规律，防止 SIMD 极简优化
          std::mt19937 rng(42);
          std::uniform_real_distribution<float> dist(-10.0f, 10.0f);

          for (int i = 0; i < AGENT_COUNT; ++i) {
            auto e = creator.create();
            writer.add<Transform>(e,
                                  Transform{{dist(rng), dist(rng), dist(rng)},
                                            {dist(rng), 0.0f, 0.0f}});
            writer.add<Velocity>(e, Velocity{{1.0f, 0.5f, 0.1f}});
            writer.add<BoidState>(
                e, BoidState{{dist(rng), dist(rng), dist(rng)}, 1.0f});
            writer.add<RenderState>(e);
            writer.add<Health>(e, Health{100.0f});
          }

          for (int i = 0; i < PARTICLE_COUNT; ++i) {
            auto e = creator.create();
            writer.add<ParticleData>(
                e, ParticleData{{dist(rng), dist(rng), dist(rng)},
                                {0.0f, 1.0f, 0.0f},
                                100.0f});
          }

          stats.get().activeEntities += (AGENT_COUNT + PARTICLE_COUNT);
          stats.get().totalSpawned += (AGENT_COUNT + PARTICLE_COUNT);
        }));
    return true;
  }

  void save(SceneBase& scene, Archive& ar) override {
    // 序列化所有组件类型
    ar.components<Transform, Velocity, BoidState, RenderState, Health,
                  ParticleData>();
  }

  void load(SceneBase& scene, Archive& ar) override {
    // 反序列化所有组件类型
    ar.components<Transform, Velocity, BoidState, RenderState, Health,
                  ParticleData>();
  }
};

class AISystem : public System {
 public:
  AISystem() : System("AISystem") {}
  bool init(SceneBase& scene) override {
    m_passes.push_back(Pass::create_update<stage::PreUpdate>(
        "AI_BoidSteer", [](ComponentReader<Transform> r_trans,
                           ComponentWriter<Velocity, BoidState> w_velboid) {
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
  bool init(SceneBase& scene) override {
    m_passes.push_back(Pass::create_update<stage::Update>(
        "Phys_UpdateAgents",
        [](ComponentWriter<Transform> w_trans, ComponentReader<Velocity> r_vel,
           ContextReader<GlobalTime> time) {
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

    m_passes.push_back(Pass::create_update<stage::Update>(
        "Phys_UpdateParticles", [](ComponentWriter<ParticleData> w_part,
                                   ContextReader<GlobalTime> time) {
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
  bool init(SceneBase& scene) override {
    m_passes.push_back(Pass::create_update<stage::PostUpdate>(
        "Render_ComputeMatrix", [](ComponentReader<Transform> r_trans,
                                   ComponentWriter<RenderState> w_rend) {
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
              math_accumulator +=
                  static_cast<uint64_t>(std::abs(r.matrix[0] * 1000.0f));
              iters++;
            }
          }
          g_total_loop_iterations.fetch_add(iters, std::memory_order_relaxed);
          g_global_state_hash.fetch_add(math_accumulator,
                                        std::memory_order_relaxed);
        }));
    return true;
  }
};

class BenchmarkMonitorSystem : public System {
 public:
  BenchmarkMonitorSystem() : System("BenchmarkMonitorSystem") {}
  bool init(SceneBase& scene) override {
    m_passes.push_back(Pass::create_update<stage::Cleanup>(
        "Checksum_Eval", [](ComponentReader<RenderState, ParticleData> reader,
                            ContextWriter<GlobalTime> time) {
          if (!time.valid()) time.create(GlobalTime{0.016f, 0});

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
// 3. 存档完整性验证工具
// ==========================================
struct SaveDataSnapshot {
  uint32_t entityCount{0};
  uint32_t transformCount{0};
  uint32_t velocityCount{0};
  uint32_t boidStateCount{0};
  uint32_t renderStateCount{0};
  uint32_t particleCount{0};
  uint32_t healthCount{0};
  float globalTimeDt{0};
  uint32_t frameCount{0};
  uint32_t activeEntities{0};
  uint32_t totalSpawned{0};

  // 计算校验和用于数据完整性验证
  size_t computeChecksum() const {
    size_t hash = 0;
    hash ^= std::hash<uint32_t>{}(entityCount);
    hash ^= std::hash<uint32_t>{}(transformCount) << 1;
    hash ^= std::hash<uint32_t>{}(velocityCount) << 2;
    hash ^= std::hash<uint32_t>{}(boidStateCount) << 3;
    hash ^= std::hash<uint32_t>{}(renderStateCount) << 4;
    hash ^= std::hash<uint32_t>{}(particleCount) << 5;
    hash ^= std::hash<uint32_t>{}(healthCount) << 6;
    hash ^= std::hash<float>{}(globalTimeDt) << 7;
    hash ^= std::hash<uint32_t>{}(frameCount) << 8;
    hash ^= std::hash<uint32_t>{}(activeEntities) << 9;
    hash ^= std::hash<uint32_t>{}(totalSpawned) << 10;
    return hash;
  }

  void print() const {
    std::cout << "  Entity Count      : " << entityCount << "\n";
    std::cout << "  Transform Count   : " << transformCount << "\n";
    std::cout << "  Velocity Count    : " << velocityCount << "\n";
    std::cout << "  BoidState Count   : " << boidStateCount << "\n";
    std::cout << "  RenderState Count : " << renderStateCount << "\n";
    std::cout << "  Particle Count    : " << particleCount << "\n";
    std::cout << "  Health Count      : " << healthCount << "\n";
    std::cout << "  GlobalTime.dt     : " << globalTimeDt << "\n";
    std::cout << "  Frame Count       : " << frameCount << "\n";
    std::cout << "  Active Entities   : " << activeEntities << "\n";
    std::cout << "  Total Spawned     : " << totalSpawned << "\n";
  }

  bool operator==(const SaveDataSnapshot& other) const {
    return entityCount == other.entityCount &&
           transformCount == other.transformCount &&
           velocityCount == other.velocityCount &&
           boidStateCount == other.boidStateCount &&
           renderStateCount == other.renderStateCount &&
           particleCount == other.particleCount &&
           healthCount == other.healthCount &&
           std::abs(globalTimeDt - other.globalTimeDt) < 0.0001f &&
           frameCount == other.frameCount &&
           activeEntities == other.activeEntities &&
           totalSpawned == other.totalSpawned;
  }
};

SaveDataSnapshot captureSnapshot(SceneBase& scene) {
  SaveDataSnapshot snapshot;
  // 使用 Transform 视图大小作为实体计数的参考（因为所有 Agent 都有 Transform）
  snapshot.entityCount =
      static_cast<uint32_t>(scene.m_registry.view<entt::entity>().size() +
                            scene.m_registry.view<ParticleData>().size());
  snapshot.transformCount =
      static_cast<uint32_t>(scene.m_registry.view<Transform>().size());
  snapshot.velocityCount =
      static_cast<uint32_t>(scene.m_registry.view<Velocity>().size());
  snapshot.boidStateCount =
      static_cast<uint32_t>(scene.m_registry.view<BoidState>().size());
  snapshot.renderStateCount =
      static_cast<uint32_t>(scene.m_registry.view<RenderState>().size());
  snapshot.particleCount =
      static_cast<uint32_t>(scene.m_registry.view<ParticleData>().size());
  snapshot.healthCount =
      static_cast<uint32_t>(scene.m_registry.view<Health>().size());

  auto timeIt = scene.m_context_manager.find(typeid(GlobalTime));
  if (timeIt != scene.m_context_manager.end()) {
    auto* time = timeIt->second.get<GlobalTime>();
    if (time) {
      snapshot.globalTimeDt = time->dt;
      snapshot.frameCount = time->frameCount;
    }
  }

  auto statsIt = scene.m_context_manager.find(typeid(GameStats));
  if (statsIt != scene.m_context_manager.end()) {
    auto* stats = statsIt->second.get<GameStats>();
    if (stats) {
      snapshot.activeEntities = stats->activeEntities;
      snapshot.totalSpawned = stats->totalSpawned;
    }
  }

  return snapshot;
}

// ==========================================
// 4. 存档/加载测试场景
// ==========================================
void runSaveLoadTest() {
  std::cout << "\n===========================================\n";
  std::cout << "      Save/Load System Test Suite         \n";
  std::cout << "===========================================\n\n";

  const stl::string savePathBinary = "./test_save.bin";
  const stl::string savePathJson = "./test_save.json";

  // ----------------------------------------
  // Phase 1: 创建初始场景并运行若干帧
  // ----------------------------------------
  std::cout << "[Phase 1] Creating initial scene...\n";

  Scene scene1;
  scene1.add_system(stl::make_shared<CoreInitSystem>());
  scene1.add_system(stl::make_shared<SpawnerSystem>());
  scene1.add_system(stl::make_shared<AISystem>());
  scene1.add_system(stl::make_shared<PhysicsSystem>());
  scene1.add_system(stl::make_shared<RenderComputeSystem>());
  scene1.add_system(stl::make_shared<BenchmarkMonitorSystem>());

  scene1.compile();
  scene1.setup();

  // 运行若干帧
  const int INITIAL_FRAMES = g_config.SAVELOAD_INIT_FRAMES;
  std::cout << "[Phase 1] Running " << INITIAL_FRAMES << " frames...\n";
  for (int i = 0; i < INITIAL_FRAMES; ++i) {
    scene1.run();
  }

  // 捕获快照
  SaveDataSnapshot snapshot1 = captureSnapshot(scene1.base());
  std::cout << "[Phase 1] Snapshot captured:\n";
  snapshot1.print();

  // ----------------------------------------
  // Phase 2: 保存到二进制文件
  // ----------------------------------------
  std::cout << "\n[Phase 2] Saving to binary file: " << savePathBinary.c_str()
            << "\n";
  scene1.save(savePathBinary);

  // 验证文件是否存在
  if (!std::filesystem::exists(savePathBinary.c_str())) {
    std::cerr << "[FAIL] Binary save file was not created!\n";
    return;
  }

  auto binaryFileSize = std::filesystem::file_size(savePathBinary.c_str());
  std::cout << "[Phase 2] Binary file size: " << binaryFileSize << " bytes\n";

  // ----------------------------------------
  // Phase 3: 保存到JSON文件
  // ----------------------------------------
  std::cout << "\n[Phase 3] Saving to JSON file: " << savePathJson.c_str()
            << "\n";
  scene1.save(savePathJson);

  if (!std::filesystem::exists(savePathJson.c_str())) {
    std::cerr << "[FAIL] JSON save file was not created!\n";
    return;
  }

  auto jsonFileSize = std::filesystem::file_size(savePathJson.c_str());
  std::cout << "[Phase 3] JSON file size: " << jsonFileSize << " bytes\n";

  // ----------------------------------------
  // Phase 4: 从二进制文件加载到新场景
  // ----------------------------------------
  std::cout << "\n[Phase 4] Loading from binary file into new scene...\n";

  Scene scene2;
  scene2.add_system(stl::make_shared<CoreInitSystem>());
  scene2.add_system(stl::make_shared<SpawnerSystem>());
  scene2.add_system(stl::make_shared<AISystem>());
  scene2.add_system(stl::make_shared<PhysicsSystem>());
  scene2.add_system(stl::make_shared<RenderComputeSystem>());
  scene2.add_system(stl::make_shared<BenchmarkMonitorSystem>());

  scene2.compile();
  scene2.load(savePathBinary);

  SaveDataSnapshot snapshot2 = captureSnapshot(scene2.base());
  std::cout << "[Phase 4] Snapshot after binary load:\n";
  snapshot2.print();

  // 验证二进制加载
  bool binaryLoadSuccess = (snapshot1 == snapshot2);
  std::cout << "\n[Phase 4] Binary Load Verification: "
            << (binaryLoadSuccess ? "PASSED" : "FAILED") << "\n";

  if (!binaryLoadSuccess) {
    std::cout << "  Expected:\n";
    snapshot1.print();
    std::cout << "  Got:\n";
    snapshot2.print();
  }

  // ----------------------------------------
  // Phase 5: 从JSON文件加载到新场景
  // ----------------------------------------
  std::cout << "\n[Phase 5] Loading from JSON file into new scene...\n";

  Scene scene3;
  scene3.add_system(stl::make_shared<CoreInitSystem>());
  scene3.add_system(stl::make_shared<SpawnerSystem>());
  scene3.add_system(stl::make_shared<AISystem>());
  scene3.add_system(stl::make_shared<PhysicsSystem>());
  scene3.add_system(stl::make_shared<RenderComputeSystem>());
  scene3.add_system(stl::make_shared<BenchmarkMonitorSystem>());

  scene3.compile();
  scene3.load(savePathJson);

  SaveDataSnapshot snapshot3 = captureSnapshot(scene3.base());
  std::cout << "[Phase 5] Snapshot after JSON load:\n";
  snapshot3.print();

  // 验证JSON加载
  bool jsonLoadSuccess = (snapshot1 == snapshot3);
  std::cout << "\n[Phase 5] JSON Load Verification: "
            << (jsonLoadSuccess ? "PASSED" : "FAILED") << "\n";

  // ----------------------------------------
  // Phase 6: 继续运行加载的场景，验证功能正常
  // ----------------------------------------
  std::cout << "\n[Phase 6] Continuing simulation from loaded scene...\n";

  const int CONTINUE_FRAMES = g_config.SAVELOAD_CONT_FRAMES;
  for (int i = 0; i < CONTINUE_FRAMES; ++i) {
    scene2.run();
  }

  SaveDataSnapshot snapshot4 = captureSnapshot(scene2.base());
  std::cout << "[Phase 6] Snapshot after " << CONTINUE_FRAMES
            << " more frames:\n";
  snapshot4.print();

  // 验证帧数是否正确增加
  bool frameContinuity =
      (snapshot4.frameCount == snapshot1.frameCount + CONTINUE_FRAMES);
  std::cout << "\n[Phase 6] Frame Continuity Check: "
            << (frameContinuity ? "PASSED" : "FAILED") << "\n";
  if (!frameContinuity) {
    std::cout << "  Expected frame count: "
              << snapshot1.frameCount + CONTINUE_FRAMES << "\n";
    std::cout << "  Actual frame count: " << snapshot4.frameCount << "\n";
  }

  // ----------------------------------------
  // Phase 7: 增量保存测试
  // ----------------------------------------
  std::cout << "\n[Phase 7] Incremental save test...\n";

  const stl::string incrementalSavePath = "./test_save_incremental.bin";
  scene2.save(incrementalSavePath);

  Scene scene4;
  scene4.add_system(stl::make_shared<CoreInitSystem>());
  scene4.add_system(stl::make_shared<SpawnerSystem>());
  scene4.add_system(stl::make_shared<AISystem>());
  scene4.add_system(stl::make_shared<PhysicsSystem>());
  scene4.add_system(stl::make_shared<RenderComputeSystem>());
  scene4.add_system(stl::make_shared<BenchmarkMonitorSystem>());

  scene4.compile();
  scene4.load(incrementalSavePath);

  SaveDataSnapshot snapshot5 = captureSnapshot(scene4.base());

  bool incrementalSuccess = (snapshot4 == snapshot5);
  std::cout << "[Phase 7] Incremental Save/Load: "
            << (incrementalSuccess ? "PASSED" : "FAILED") << "\n";

  // ----------------------------------------
  // Phase 8: 组件数据完整性抽样验证
  // ----------------------------------------
  std::cout << "\n[Phase 8] Component data integrity sampling...\n";

  bool componentIntegrity = true;
  int sampleCount = 0;
  int maxSamples = g_config.MAX_SAMPLES;

  // 抽样验证Transform组件数据 (比较 scene2 和 scene4，增量保存前后)
  auto view2 = scene2.base().m_registry.view<Transform>();
  auto view4 = scene4.base().m_registry.view<Transform>();

  if (view2.size() == view4.size() && view2.size() > 0) {
    auto it2 = view2.begin();
    auto it4 = view4.begin();

    while (it2 != view2.end() && it4 != view4.end() &&
           sampleCount < maxSamples) {
      const auto& t2 = view2.get<Transform>(*it2);
      const auto& t4 = view4.get<Transform>(*it4);

      for (int i = 0; i < 3; ++i) {
        if (std::abs(t2.pos[i] - t4.pos[i]) > 0.0001f) {
          componentIntegrity = false;
          break;
        }
        if (std::abs(t2.rot[i] - t4.rot[i]) > 0.0001f) {
          componentIntegrity = false;
          break;
        }
      }

      if (!componentIntegrity) break;

      ++it2;
      ++it4;
      ++sampleCount;
    }
  } else {
    componentIntegrity = false;
  }

  std::cout << "[Phase 8] Sampled " << sampleCount << " Transform components\n";
  std::cout << "[Phase 8] Component Data Integrity: "
            << (componentIntegrity ? "PASSED" : "FAILED") << "\n";

  // ----------------------------------------
  // Final Summary
  // ----------------------------------------
  std::cout << "\n===========================================\n";
  std::cout << "           TEST SUMMARY                    \n";
  std::cout << "===========================================\n";
  std::cout << " Binary Load Test     : "
            << (binaryLoadSuccess ? "PASSED" : "FAILED") << "\n";
  std::cout << " JSON Load Test       : "
            << (jsonLoadSuccess ? "PASSED" : "FAILED") << "\n";
  std::cout << " Frame Continuity     : "
            << (frameContinuity ? "PASSED" : "FAILED") << "\n";
  std::cout << " Incremental Save     : "
            << (incrementalSuccess ? "PASSED" : "FAILED") << "\n";
  std::cout << " Component Integrity  : "
            << (componentIntegrity ? "PASSED" : "FAILED") << "\n";
  std::cout << "-------------------------------------------\n";

  bool allPassed = binaryLoadSuccess && jsonLoadSuccess && frameContinuity &&
                   incrementalSuccess && componentIntegrity;
  std::cout << " Overall Result       : "
            << (allPassed ? "ALL TESTS PASSED" : "SOME TESTS FAILED") << "\n";
  std::cout << "===========================================\n";

  // 清理测试文件
  std::cout << "\nCleaning up test files...\n";
  std::filesystem::remove(savePathBinary.c_str());
  std::filesystem::remove(savePathJson.c_str());
  std::filesystem::remove(incrementalSavePath.c_str());
  std::cout << "Cleanup complete.\n";
}

// ==========================================
// 5. 严格受控测试启动点
// ==========================================
int main() {
  std::cout << "===========================================\n";
  std::cout << " FantasyEngine ECS - Ironclad Benchmark  \n";
  std::cout << "===========================================\n\n";

  // 打印当前测试配置
  g_config.print();
  std::cout << "\n";

  try {
    Scene scene;

    scene.add_system(stl::make_shared<CoreInitSystem>());
    scene.add_system(stl::make_shared<SpawnerSystem>());
    scene.add_system(stl::make_shared<AISystem>());
    scene.add_system(stl::make_shared<PhysicsSystem>());
    scene.add_system(stl::make_shared<RenderComputeSystem>());
    scene.add_system(stl::make_shared<BenchmarkMonitorSystem>());

    std::cout << "[1] Compiling execution graph...\n";
    scene.compile();
    scene.dump_graph("./");

    std::cout << "[2] Running Setup Phase...\n";
    scene.setup();

    // 关键断言！如果此处等于 0，证明 Taskflow 在执行 Setup 阶段时失效！
    if (g_setup_executions.load() == 0) {
      std::cerr
          << ">>> [FATAL] Setup Phase NEVER EXECUTED! The Registry is empty!\n";
      std::cerr
          << ">>> Please check Taskflow dispatch logic for Setup passes.\n";
      return EXIT_FAILURE;
    }

    const int TEST_FRAMES = g_config.BENCHMARK_FRAMES;
    std::cout << "[3] Running Benchmark for " << TEST_FRAMES
              << " frames...\n\n";

    std::vector<double> frameTimes;
    frameTimes.reserve(TEST_FRAMES);

    auto testStartTime = std::chrono::high_resolution_clock::now();

    for (int frame = 0; frame < TEST_FRAMES; ++frame) {
      auto frameStart = std::chrono::high_resolution_clock::now();

      scene.run();

      auto frameEnd = std::chrono::high_resolution_clock::now();
      double frameTimeMs =
          std::chrono::duration<double, std::milli>(frameEnd - frameStart)
              .count();
      frameTimes.push_back(frameTimeMs);

      if ((frame + 1) % 100 == 0) {
        std::cout << "[Frame " << std::setw(3) << (frame + 1) << "] "
                  << "Time: " << std::fixed << std::setprecision(2)
                  << std::setw(6) << frameTimeMs << " ms "
                  << "(" << std::setw(5)
                  << static_cast<int>(1000.0 / frameTimeMs) << " FPS)\n";
      }
    }

    auto testEndTime = std::chrono::high_resolution_clock::now();
    double totalTestTime =
        std::chrono::duration<double>(testEndTime - testStartTime).count();

    double minTime = frameTimes[0], maxTime = frameTimes[0], sumTime = 0.0;
    for (double t : frameTimes) {
      if (t < minTime) minTime = t;
      if (t > maxTime) maxTime = t;
      sumTime += t;
    }
    double avgTime = sumTime / TEST_FRAMES;

    std::cout << "\n===========================================\n";
    std::cout << "             IRONCLAD RESULTS              \n";
    std::cout << "===========================================\n";
    std::cout << " Total Frames     : " << TEST_FRAMES << "\n";
    std::cout << " Total Time (s)   : " << std::fixed << std::setprecision(3)
              << totalTestTime << " s\n";
    std::cout << " Max Frame Time   : " << maxTime << " ms\n";
    std::cout << " Min Frame Time   : " << minTime << " ms\n";
    std::cout << " Avg Frame Time   : " << avgTime << " ms\n";
    std::cout << " Avg FPS          : " << static_cast<int>(1000.0 / avgTime)
              << "\n";
    std::cout << "-------------------------------------------\n";
    std::cout << " Iterations Exec  : " << g_total_loop_iterations.load()
              << " loops\n";
    std::cout << " Operations Hash  : " << g_global_state_hash.load() << "\n";
    std::cout << "===========================================\n";

    // 如果迭代次数为 0，说明视图内根本没东西
    if (g_total_loop_iterations.load() == 0) {
      std::cerr << "\n>>> [WARNING] 0 iterations detected! The Views are EMPTY "
                   "during update.\n";
      std::cerr << ">>> Reason: Either `m_reg.emplace_or_replace` failed "
                   "silently, or Entity creation didn't persist.\n";
    }

  } catch (const std::exception& e) {
    std::cerr << "\n[FATAL ERROR] " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  // ==========================================
  // 运行存档/加载测试
  // ==========================================
  try {
    runSaveLoadTest();
  } catch (const std::exception& e) {
    std::cerr << "\n[SAVE/LOAD TEST ERROR] " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}