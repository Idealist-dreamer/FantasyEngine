#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <memory>
#include <vector>
#include <random>

// 包含ECS框架头文件
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
// 1. 定义数据层：组件 (Components)、资源 (Resources)、事件 (Events)
// ==========================================

struct Position {
  float x, y, z;
};

struct Velocity {
  float dx, dy, dz;
};

struct Transform {
  float matrix[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
};

struct Health {
  int current;
  int max;
};

// 用于测试并行性能的高负载数据
struct HeavyDataA {
  float data[64] = {0};
};

struct HeavyDataB {
  float data[64] = {0};
};

struct HeavyDataC {
  float data[64] = {0};
};

struct TimeResource {
  float deltaTime;
  float totalTime;
};

struct FrameCounter {
  uint64_t frameCount = 0;
};

struct CollisionEvent {
  uint32_t entityA, entityB;
  float impactForce;
};

struct ParticleEvent {
  uint32_t entity;
  float x, y, z;
};

// ==========================================
// 2. 定义逻辑层：系统 (Systems)
// ==========================================

// 初始化系统：在Init阶段创建实体和初始化资源
class InitSystem : public System {
 public:
  InitSystem() : System("InitSystem") {}

  bool init() override {
    m_passes.push_back(Pass::create_start<stage::Init>("InitResources", [](ResourceWriter<TimeResource> timeResWriter) {
      // 初始化时间资源
      timeResWriter.create(TimeResource{0.016f, 0.0f});
      std::cout << "[InitSystem] Initialized time resource\n";
    }));
    return true;
  }
};

// 实体创建系统：在Startup阶段批量创建实体
class EntityCreationSystem : public System {
 public:
  EntityCreationSystem() : System("EntityCreationSystem") {}

  bool init() override {
    m_passes.push_back(Pass::create_start<stage::Startup>(
        "CreateEntities",
        [](EntityCreator creator, ComponentWriter<Position, Velocity, Transform, Health, HeavyDataA, HeavyDataB, HeavyDataC> writer) {
          // 批量创建10000个实体
          for (int i = 0; i < 10000; ++i) {
            auto e = creator.create();
            writer.add<Position>(e, static_cast<float>(i), static_cast<float>(i % 100), 0.0f);
            writer.add<Velocity>(e, 1.0f + (i % 5) * 0.1f, 0.5f + (i % 3) * 0.2f, 0.0f);
            writer.add<Transform>(e);
            writer.add<Health>(e, 100, 100);
            writer.add<HeavyDataA>(e);
            writer.add<HeavyDataB>(e);
            writer.add<HeavyDataC>(e);
          }
          std::cout << "[EntityCreationSystem] Created 10,000 entities\n";
        }));
    return true;
  }
};

// 运动系统：更新位置和变换
class MovementSystem : public System {
 public:
  MovementSystem() : System("MovementSystem") {}

  bool init() override {
    // 第一阶段：更新位置
    m_passes.push_back(Pass::create_update<stage::Update>(
        "UpdatePositions", [](ComponentWriter<Position> posWriter, ComponentReader<Velocity> velReader, ResourceReader<TimeResource> timeRes) {
          float dt = timeRes.get().deltaTime;
          for (auto e : posWriter.view()) {
            if (velReader.have<Velocity>(e)) {
              auto& pos = posWriter.get<Position>(e);
              const auto& vel = velReader.get<Velocity>(e);
              pos.x += vel.dx * dt;
              pos.y += vel.dy * dt;
              pos.z += vel.dz * dt;
            }
          }
        }));

    // 第二阶段：更新变换矩阵
    m_passes.push_back(
        Pass::create_update<stage::Update>("UpdateTransforms", [](ComponentWriter<Transform> transformWriter, ComponentReader<Position> posReader) {
          for (auto e : transformWriter.view()) {
            if (posReader.have<Position>(e)) {
              auto& transform = transformWriter.get<Transform>(e);
              const auto& pos = posReader.get<Position>(e);
              transform.matrix[12] = pos.x;
              transform.matrix[13] = pos.y;
              transform.matrix[14] = pos.z;
            }
          }
        }));

    return true;
  }
};

// 高负载计算系统A：处理HeavyDataA
class HeavyComputeSystemA : public System {
 public:
  HeavyComputeSystemA() : System("HeavyComputeSystemA") {}

  bool init() override {
    m_passes.push_back(Pass::create_update<stage::Update>("ProcessHeavyDataA", [](ComponentWriter<HeavyDataA> writer) {
      static std::random_device rd;
      static std::mt19937 gen(rd());
      static std::uniform_real_distribution<float> dis(-1.0f, 1.0f);

      for (auto e : writer.view()) {
        auto& data = writer.get<HeavyDataA>(e);
        for (int i = 0; i < 64; ++i) {
          data.data[i] = std::sin(data.data[i] + dis(gen) * 0.01f);
        }
      }
    }));
    return true;
  }
};

// 高负载计算系统B：处理HeavyDataB
class HeavyComputeSystemB : public System {
 public:
  HeavyComputeSystemB() : System("HeavyComputeSystemB") {}

  bool init() override {
    m_passes.push_back(Pass::create_update<stage::Update>("ProcessHeavyDataB", [](ComponentWriter<HeavyDataB> writer) {
      static std::random_device rd;
      static std::mt19937 gen(rd());
      static std::uniform_real_distribution<float> dis(-1.0f, 1.0f);

      for (auto e : writer.view()) {
        auto& data = writer.get<HeavyDataB>(e);
        for (int i = 0; i < 64; ++i) {
          data.data[i] = std::cos(data.data[i] + dis(gen) * 0.01f);
        }
      }
    }));
    return true;
  }
};

// 高负载计算系统C：处理HeavyDataC
class HeavyComputeSystemC : public System {
 public:
  HeavyComputeSystemC() : System("HeavyComputeSystemC") {}

  bool init() override {
    m_passes.push_back(Pass::create_update<stage::Update>("ProcessHeavyDataC", [](ComponentWriter<HeavyDataC> writer) {
      static std::random_device rd;
      static std::mt19937 gen(rd());
      static std::uniform_real_distribution<float> dis(-1.0f, 1.0f);

      for (auto e : writer.view()) {
        auto& data = writer.get<HeavyDataC>(e);
        for (int i = 0; i < 64; ++i) {
          data.data[i] = std::tan(data.data[i] + dis(gen) * 0.01f);
        }
      }
    }));
    return true;
  }
};

// 事件发射系统：在PreUpdate阶段发射事件
class EventEmitSystem : public System {
 public:
  EventEmitSystem() : System("EventEmitSystem") {}

  bool init() override {
    m_passes.push_back(Pass::create_update<stage::PreUpdate>(
        "EmitCollisionEvents", [](EventWriter<CollisionEvent> eventWriter, ResourceReader<FrameCounter> frameCounter) {
          // 每帧发射一些碰撞事件
          for (int i = 0; i < 100; ++i) {
            eventWriter.get().push_back({static_cast<uint32_t>((frameCounter.get().frameCount + i) % 10000),
                                         static_cast<uint32_t>((frameCounter.get().frameCount + i + 1) % 10000), 10.0f + static_cast<float>(i)});
          }
          std::cout << "[EventEmitSystem] Emitted 100 collision events\n";
        }));

    m_passes.push_back(
        Pass::create_update<stage::PreUpdate>("EmitParticleEvents", [](EventWriter<ParticleEvent> eventWriter, ComponentReader<Position> posReader) {
          // 为每个有位置的实体发射粒子事件
          int count = 0;
          for (auto e : posReader.view()) {
            if (count++ >= 500)
              break;  // 限制数量
            const auto& pos = posReader.get<Position>(e);
            eventWriter.get().push_back({static_cast<uint32_t>(e), pos.x, pos.y, pos.z});
          }
          std::cout << "[EventEmitSystem] Emitted " << count << " particle events\n";
        }));

    return true;
  }
};

// 事件处理系统：在PostUpdate阶段处理事件
class EventProcessSystem : public System {
 public:
  EventProcessSystem() : System("EventProcessSystem") {}

  bool init() override {
    m_passes.push_back(Pass::create_update<stage::PostUpdate>("ProcessCollisionEvents", [](EventReader<CollisionEvent> eventReader) {
      int processed = 0;
      for (const auto& event : eventReader.get()) {
        // 模拟碰撞处理逻辑
        processed++;
      }
      std::cout << "[EventProcessSystem] Processed " << processed << " collision events\n";
    }));

    m_passes.push_back(Pass::create_update<stage::PostUpdate>("ProcessParticleEvents", [](EventReader<ParticleEvent> eventReader) {
      int processed = 0;
      for (const auto& event : eventReader.get()) {
        // 模拟粒子处理逻辑
        processed++;
      }
      std::cout << "[EventProcessSystem] Processed " << processed << " particle events\n";
    }));

    return true;
  }
};

// 统计系统：在Last阶段输出统计信息
class StatisticsSystem : public System {
 public:
  StatisticsSystem() : System("StatisticsSystem") {}

  bool init() override {
    m_passes.push_back(Pass::create_update<stage::Last>(
        "UpdateStatistics",
        [](ResourceWriter<TimeResource> timeResWriter, ResourceWriter<FrameCounter> frameCounterWriter, ComponentReader<Position> posReader) {
          // 更新时间和帧计数器
          auto& timeRes = timeResWriter.get();
          auto& frameCounter = frameCounterWriter.get();

          timeRes.totalTime += timeRes.deltaTime;
          frameCounter.frameCount++;

          if (frameCounter.frameCount % 10 == 0) {
            std::cout << "[StatisticsSystem] Frame: " << frameCounter.frameCount << ", Total time: " << timeRes.totalTime
                      << ", Active entities: " << posReader.view().size() << "\n";
          }
        }));
    return true;
  }
};

// ==========================================
// 3. 主程序：测试框架功能
// ==========================================

int main() {
  std::cout << "=== FantasyEngine ECS Framework Test ===\n";

  try {
    // 创建World实例
    World world;

    // 创建并添加多个系统
    auto initSystem = stl::make_shared<InitSystem>();
    auto entityCreationSystem = stl::make_shared<EntityCreationSystem>();
    auto movementSystem = stl::make_shared<MovementSystem>();
    auto heavySystemA = stl::make_shared<HeavyComputeSystemA>();
    auto heavySystemB = stl::make_shared<HeavyComputeSystemB>();
    auto heavySystemC = stl::make_shared<HeavyComputeSystemC>();
    auto eventEmitSystem = stl::make_shared<EventEmitSystem>();
    auto eventProcessSystem = stl::make_shared<EventProcessSystem>();
    auto statisticsSystem = stl::make_shared<StatisticsSystem>();

    // 添加系统到World
    world.add_system(initSystem);
    world.add_system(entityCreationSystem);
    world.add_system(movementSystem);
    world.add_system(heavySystemA);
    world.add_system(heavySystemB);
    world.add_system(heavySystemC);
    world.add_system(eventEmitSystem);
    world.add_system(eventProcessSystem);
    world.add_system(statisticsSystem);

    // 编译任务流
    std::cout << "\n=== Compiling taskflow ===\n";
    auto compileStart = std::chrono::high_resolution_clock::now();
    world.compile();
    auto compileEnd = std::chrono::high_resolution_clock::now();
    auto compileTime = std::chrono::duration_cast<std::chrono::milliseconds>(compileEnd - compileStart);
    std::cout << "Taskflow compilation time: " << compileTime.count() << "ms\n";

    // 可选：导出任务图用于可视化
    world.dump_graph("taskflow_graph.dot");

    // 设置World
    world.setup();

    // 运行多帧测试
    std::cout << "\n=== Running frame simulation ===\n";
    const int FRAME_COUNT = 30;

    std::vector<long long> frameTimes;
    frameTimes.reserve(FRAME_COUNT);

    for (int frame = 0; frame < FRAME_COUNT; ++frame) {
      auto frameStart = std::chrono::high_resolution_clock::now();

      // 执行一帧
      world.run();

      auto frameEnd = std::chrono::high_resolution_clock::now();
      auto frameTime = std::chrono::duration_cast<std::chrono::microseconds>(frameEnd - frameStart);
      frameTimes.push_back(frameTime.count());

      // 每10帧输出一次性能统计
      if (frame % 10 == 9) {
        long long totalTime = 0;
        for (auto time : frameTimes) {
          totalTime += time;
        }
        double avgFrameTime = static_cast<double>(totalTime) / frameTimes.size();

        std::cout << "[Performance] Frame " << frame + 1 << " - Avg frame time: " << avgFrameTime << "μs"
                  << " (" << 1000000.0 / avgFrameTime << " FPS)\n";
      }

      // 模拟帧间延迟
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    // 性能分析
    std::cout << "\n=== Performance Analysis ===\n";

    long long minTime = *std::min_element(frameTimes.begin(), frameTimes.end());
    long long maxTime = *std::max_element(frameTimes.begin(), frameTimes.end());
    long long totalTime = 0;

    for (auto time : frameTimes) {
      totalTime += time;
    }

    double avgFrameTime = static_cast<double>(totalTime) / frameTimes.size();
    double fps = 1000000.0 / avgFrameTime;

    std::cout << "Total frames: " << FRAME_COUNT << "\n";
    std::cout << "Min frame time: " << minTime << "μs\n";
    std::cout << "Max frame time: " << maxTime << "μs\n";
    std::cout << "Avg frame time: " << avgFrameTime << "μs\n";
    std::cout << "Avg FPS: " << fps << "\n";
    std::cout << "Total execution time: " << totalTime / 1000.0 << "ms\n";

    std::cout << "\n=== Test completed successfully ===\n";

  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}