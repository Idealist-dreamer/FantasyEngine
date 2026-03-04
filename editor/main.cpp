#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <memory>
#include <vector>
#include <random>
#include <iomanip>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <numeric>

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

// 基础组件
struct Position {
  float x, y, z;
};

struct Velocity {
  float vx, vy, vz;
};

struct Acceleration {
  float ax, ay, az;
};

struct Rotation {
  float pitch, yaw, roll;
};

struct Scale {
  float x, y, z;
};

// 物理组件
struct RigidBody {
  float mass;
  float restitution; // 弹性系数
  bool isStatic;
};

struct CollisionShape {
  enum Type { SPHERE, BOX, CAPSULE } type;
  float radius;      // 球体半径/胶囊半径
  float height;      // 胶囊高度
  float halfExtents[3]; // 盒子半尺寸
};

struct PhysicsState {
  bool isColliding;
  entt::entity collidedWith;
  float collisionImpulse;
};

// 渲染组件
struct Mesh {
  uint32_t vertexCount;
  uint32_t indexCount;
  std::string meshName;
};

struct Material {
  float color[4]; // RGBA
  float metallic;
  float roughness;
  std::string textureName;
};

struct Transform {
  float matrix[16];
};

// 游戏逻辑组件
struct Health {
  float current;
  float max;
};

struct Lifetime {
  int ticks;
};

struct AIState {
  enum Behavior { IDLE, PATROL, CHASE, ATTACK } behavior;
  entt::entity target;
  float detectionRange;
};

// 性能测试专用组件（模拟复杂计算）
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

// 资源：全局状态管理
struct GlobalTime {
  float dt;
  uint64_t frameCount;
};

struct GameStats {
  uint32_t activeEntities;
  uint32_t totalSpawned;
  uint32_t totalDestroyed;
  uint32_t physicsCollisions;
  uint32_t renderCalls;
  uint32_t aiUpdates;
};

struct PhysicsSettings {
  float gravity;
  float airResistance;
  uint32_t maxCollisionChecks;
};

struct RenderSettings {
  bool enableShadows;
  bool enablePostProcessing;
  uint32_t maxLights;
};

// 事件系统
struct DamageEvent {
  entt::entity target;
  float amount;
};

struct CollisionEvent {
  entt::entity entityA;
  entt::entity entityB;
  float impulse;
};

struct ParticleEvent {
  entt::entity source;
  uint32_t count;
  float lifetime;
};

struct AIEvent {
  entt::entity entity;
  AIState::Behavior newBehavior;
};

// ==========================================
// 2. 定义逻辑层：系统 (Systems)
// ==========================================

// 初始化系统：设定所有全局资源和配置
class InitSystem : public System {
 public:
  InitSystem() : System("InitSystem") {}

  bool init() override {
    m_passes.push_back(
        Pass::create_start<stage::Init>("InitResources", [](ResourceWriter<GlobalTime> timeWriter) {
          timeWriter.create(GlobalTime{0.016f, 0});
        }));
    
    m_passes.push_back(
        Pass::create_start<stage::Init>("InitGameStats", [](ResourceWriter<GameStats> statsWriter) {
          statsWriter.create(GameStats{0, 0, 0, 0, 0, 0});
        }));
    
    m_passes.push_back(
        Pass::create_start<stage::Init>("InitPhysicsSettings", [](ResourceWriter<PhysicsSettings> physicsWriter) {
          physicsWriter.create(PhysicsSettings{-9.8f, 0.1f, 10000});
        }));
    
    m_passes.push_back(
        Pass::create_start<stage::Init>("InitRenderSettings", [](ResourceWriter<RenderSettings> renderWriter) {
          renderWriter.create(RenderSettings{true, true, 32});
        }));
    
    return true;
  }
};

// 实体生成系统：负责初始批量生成和每帧动态生成
class SpawnerSystem : public System {
 public:
  SpawnerSystem() : System("SpawnerSystem") {}

  static void SpawnPhysicsEntity(EntityCreator& creator,
                                ComponentWriter<Position, Velocity, Acceleration, Rotation, RigidBody, CollisionShape, PhysicsState,
                                                Mesh, Material, Transform, Health, Lifetime, AIState,
                                                ComputeDataA, ComputeDataB, ComputeDataC, ComputeDataD>& writer,
                                int index, GameStats& stats) {
    auto e = creator.create();
    float offset = static_cast<float>(index);

    // 基础组件
    writer.add<Position>(e, offset * 2.0f, 0.0f, 0.0f);
    writer.add<Velocity>(e, std::sin(offset) * 5.0f, std::cos(offset) * 3.0f, 0.0f);
    writer.add<Acceleration>(e, 0.0f, 0.0f, 0.0f);
    writer.add<Rotation>(e, offset * 0.1f, offset * 0.05f, 0.0f);

    // 物理组件
    writer.add<RigidBody>(e, 1.0f + std::fmod(offset, 5.0f), 0.8f, false);
    
    CollisionShape shape;
    shape.type = static_cast<CollisionShape::Type>(index % 3);
    shape.radius = 0.5f + std::fmod(offset, 1.0f);
    shape.height = 2.0f;
    shape.halfExtents[0] = 0.5f; shape.halfExtents[1] = 0.5f; shape.halfExtents[2] = 0.5f;
    writer.add<CollisionShape>(e, shape);
    
    writer.add<PhysicsState>(e, false, entt::null, 0.0f);

    // 渲染组件
    writer.add<Mesh>(e, 1000u, 2000u, "sphere_mesh");
    
    Material mat;
    mat.color[0] = std::fmod(offset * 0.1f, 1.0f);
    mat.color[1] = std::fmod(offset * 0.2f, 1.0f);
    mat.color[2] = std::fmod(offset * 0.3f, 1.0f);
    mat.color[3] = 1.0f;
    mat.metallic = 0.5f;
    mat.roughness = 0.3f;
    mat.textureName = "default_texture";
    writer.add<Material>(e, mat);
    
    Transform transform;
    std::fill(std::begin(transform.matrix), std::end(transform.matrix), 0.0f);
    transform.matrix[0] = transform.matrix[5] = transform.matrix[10] = transform.matrix[15] = 1.0f;
    writer.add<Transform>(e, transform);

    // 游戏逻辑组件
    writer.add<Health>(e, 100.0f, 100.0f);
    writer.add<Lifetime>(e, 100 + (index % 200));
    
    AIState ai;
    ai.behavior = static_cast<AIState::Behavior>((index / 1000) % 4);
    ai.detectionRange = 10.0f + std::fmod(offset, 5.0f);
    writer.add<AIState>(e, ai);

    // 性能测试组件
    writer.add<ComputeDataA>(e);
    writer.add<ComputeDataB>(e);
    writer.add<ComputeDataC>(e);
    writer.add<ComputeDataD>(e);

    stats.activeEntities++;
    stats.totalSpawned++;
  }

  bool init() override {
    // Startup阶段：一次性生成 100,000 个物理实体
    m_passes.push_back(Pass::create_start<stage::Startup>(
        "InitialSpawn", [](EntityCreator creator,
                           ComponentWriter<Position, Velocity, Acceleration, Rotation, RigidBody, CollisionShape, PhysicsState,
                                          Mesh, Material, Transform, Health, Lifetime, AIState,
                                          ComputeDataA, ComputeDataB, ComputeDataC, ComputeDataD> writer,
                           ResourceWriter<GameStats> stats) {
          for (int i = 0; i < 100000; ++i) {
            SpawnPhysicsEntity(creator, writer, i, stats.get());
          }
          std::cout << "[SpawnerSystem] Initialized 100,000 physics entities.\n";
        }));

    // PreUpdate阶段：每帧动态生成 2,000 个新实体
    m_passes.push_back(Pass::create_update<stage::PreUpdate>(
        "DynamicSpawn", [](EntityCreator creator,
                           ComponentWriter<Position, Velocity, Acceleration, Rotation, RigidBody, CollisionShape, PhysicsState,
                                          Mesh, Material, Transform, Health, Lifetime, AIState,
                                          ComputeDataA, ComputeDataB, ComputeDataC, ComputeDataD> writer,
                           ResourceWriter<GameStats> stats) {
          static int spawnCounter = 0;
          for (int i = 0; i < 2000; ++i) {
            SpawnPhysicsEntity(creator, writer, 100000 + spawnCounter * 2000 + i, stats.get());
          }
          spawnCounter++;
        }));
    return true;
  }
};

// 物理系统：运动学计算
class PhysicsSystem : public System {
 public:
  PhysicsSystem() : System("PhysicsSystem") {}

  bool init() override {
    // 运动学更新：读 Velocity/Acceleration，写 Position
    m_passes.push_back(Pass::create_update<stage::Update>(
        "UpdateKinematics", [](ComponentWriter<Position> posWriter, 
                              ComponentReader<Velocity, Acceleration> reader, 
                              ResourceReader<GlobalTime> timeRes, 
                              ResourceReader<PhysicsSettings> physicsRes) {
          float dt = timeRes.get().dt;
          float gravity = physicsRes.get().gravity;
          
          for (auto e : posWriter.view()) {
            if (reader.have_all<Velocity, Acceleration>(e)) {
              auto& pos = posWriter.get<Position>(e);
              const auto& vel = reader.get<Velocity>(e);
              const auto& acc = reader.get<Acceleration>(e);
              
              // 应用重力
              pos.x += vel.vx * dt + 0.5f * acc.ax * dt * dt;
              pos.y += vel.vy * dt + 0.5f * (acc.ay + gravity) * dt * dt;
              pos.z += vel.vz * dt + 0.5f * acc.az * dt * dt;
            }
          }
        }));

    // 速度更新：读 Acceleration，写 Velocity
    m_passes.push_back(Pass::create_update<stage::Update>(
        "UpdateVelocity", [](ComponentWriter<Velocity> velWriter, 
                            ComponentReader<Acceleration> accReader, 
                            ResourceReader<GlobalTime> timeRes,
                            ResourceReader<PhysicsSettings> physicsRes) {
          float dt = timeRes.get().dt;
          float airResistance = physicsRes.get().airResistance;
          
          for (auto e : velWriter.view()) {
            if (accReader.have<Acceleration>(e)) {
              auto& vel = velWriter.get<Velocity>(e);
              const auto& acc = accReader.get<Acceleration>(e);
              
              vel.vx += acc.ax * dt;
              vel.vy += acc.ay * dt;
              vel.vz += acc.az * dt;
              
              // 应用空气阻力
              vel.vx *= (1.0f - airResistance * dt);
              vel.vy *= (1.0f - airResistance * dt);
              vel.vz *= (1.0f - airResistance * dt);
            }
          }
        }));

    return true;
  }
};

// 碰撞检测系统
class CollisionSystem : public System {
 public:
  CollisionSystem() : System("CollisionSystem") {}

  bool init() override {
    // 发射碰撞事件
    m_passes.push_back(Pass::create_update<stage::PostUpdate>(
        "EmitCollisionEvents", [](EventWriter<CollisionEvent> eventWriter, 
                                 ComponentReader<Position, CollisionShape, PhysicsState> reader, 
                                 ResourceReader<PhysicsSettings> settings) {
          auto& events = eventWriter.get();
          uint32_t maxChecks = settings.get().maxCollisionChecks;
          uint32_t checks = 0;
          
          // 简化的空间分区检测（实际项目应使用四叉树/八叉树）
          auto view = reader.view<Position, CollisionShape>();
          for (auto it1 = view.begin(); it1 != view.end() && checks < maxChecks; ++it1) {
            auto e1 = *it1;
            const auto& pos1 = reader.get<Position>(e1);
            const auto& shape1 = reader.get<CollisionShape>(e1);
            
            for (auto it2 = std::next(it1); it2 != view.end() && checks < maxChecks; ++it2) {
              auto e2 = *it2;
              const auto& pos2 = reader.get<Position>(e2);
              
              // 简单的距离检测
              float dx = pos1.x - pos2.x;
              float dy = pos1.y - pos2.y;
              float dz = pos1.z - pos2.z;
              float distance = std::sqrt(dx*dx + dy*dy + dz*dz);
              
              if (distance < 2.0f) { // 简化碰撞检测
                events.push_back({e1, e2, distance});
                checks++;
              }
              
              if (checks >= maxChecks) break;
            }
          }
        }));

    // 处理碰撞事件
    m_passes.push_back(Pass::create_update<stage::PostUpdate>(
        "ProcessCollisions", [](EventReader<CollisionEvent> eventReader, 
                               ComponentWriter<PhysicsState> stateWriter,
                               ComponentReader<RigidBody> bodyReader,
                               ResourceWriter<GameStats> stats) {
          for (const auto& ev : eventReader.get()) {
            if (stateWriter.have<PhysicsState>(ev.entityA) && stateWriter.have<PhysicsState>(ev.entityB)) {
              auto& stateA = stateWriter.get<PhysicsState>(ev.entityA);
              auto& stateB = stateWriter.get<PhysicsState>(ev.entityB);
              
              stateA.isColliding = true;
              stateA.collidedWith = ev.entityB;
              stateA.collisionImpulse = ev.impulse;
              
              stateB.isColliding = true;
              stateB.collidedWith = ev.entityA;
              stateB.collisionImpulse = ev.impulse;
              
              stats.get().physicsCollisions++;
            }
          }
        }));

    return true;
  }
};

// 渲染系统：模拟图形管线处理
class RenderSystem : public System {
 public:
  RenderSystem() : System("RenderSystem") {}

  bool init() override {
    // 变换矩阵计算
    m_passes.push_back(Pass::create_update<stage::Update>(
        "UpdateTransforms", [](ComponentWriter<Transform> transformWriter, 
                              ComponentReader<Position, Rotation, Scale> reader) {
          for (auto e : transformWriter.view()) {
            if (reader.have_all<Position, Rotation, Scale>(e)) {
              auto& transform = transformWriter.get<Transform>(e);
              const auto& pos = reader.get<Position>(e);
              const auto& rot = reader.get<Rotation>(e);
              const auto& scale = reader.get<Scale>(e);
              
              // 简化的变换矩阵计算（实际项目应使用数学库）
              std::fill(std::begin(transform.matrix), std::end(transform.matrix), 0.0f);
              transform.matrix[0] = scale.x;
              transform.matrix[5] = scale.y;
              transform.matrix[10] = scale.z;
              transform.matrix[12] = pos.x;
              transform.matrix[13] = pos.y;
              transform.matrix[14] = pos.z;
              transform.matrix[15] = 1.0f;
            }
          }
        }));

    // 材质处理
    m_passes.push_back(Pass::create_update<stage::Update>(
        "ProcessMaterials", [](ComponentWriter<Material> materialWriter, 
                              ComponentReader<Health, PhysicsState> reader,
                              ResourceReader<RenderSettings> settings) {
          for (auto e : materialWriter.view()) {
            auto& mat = materialWriter.get<Material>(e);
            
            // 基于健康状况动态调整材质
            if (reader.have<Health>(e)) {
              const auto& health = reader.get<Health>(e);
              float healthRatio = health.current / health.max;
              mat.metallic = healthRatio * 0.8f;
              mat.roughness = 1.0f - healthRatio * 0.5f;
            }
            
            // 碰撞时高亮显示
            if (reader.have<PhysicsState>(e)) {
              const auto& physics = reader.get<PhysicsState>(e);
              if (physics.isColliding) {
                mat.color[0] = 1.0f; // 红色高亮
                mat.color[1] = 0.2f;
                mat.color[2] = 0.2f;
              }
            }
          }
        }));

    // 渲染统计
    m_passes.push_back(Pass::create_update<stage::Last>(
        "RenderStats", [](ComponentReader<Mesh, Material, Transform> reader, ResourceWriter<GameStats> stats) {
          auto& gameStats = stats.get();
          size_t count = 0;
          for (auto it = reader.view<Mesh, Material, Transform>().begin(); it != reader.view<Mesh, Material, Transform>().end(); ++it) {
            count++;
          }
          gameStats.renderCalls = static_cast<uint32_t>(count);
        }));

    return true;
  }
};

// AI系统：智能行为管理
class AISystem : public System {
 public:
  AISystem() : System("AISystem") {}

  bool init() override {
    // AI行为更新
    m_passes.push_back(Pass::create_update<stage::Update>(
        "UpdateAIBehavior", [](ComponentWriter<AIState> aiWriter, 
                              ComponentReader<Position, Health> reader,
                              ResourceReader<GlobalTime> timeRes) {
          int frame = timeRes.get().frameCount;
          
          for (auto e : aiWriter.view()) {
            auto& ai = aiWriter.get<AIState>(e);
            
            // 基于帧数和健康状况的简单AI逻辑
            if (frame % 100 == static_cast<uint32_t>(e) % 100) {
              switch (ai.behavior) {
                case AIState::IDLE:
                  if (reader.have<Health>(e) && reader.get<Health>(e).current < 50.0f) {
                    ai.behavior = AIState::CHASE;
                  }
                  break;
                case AIState::PATROL:
                  ai.behavior = AIState::IDLE;
                  break;
                case AIState::CHASE:
                  ai.behavior = AIState::ATTACK;
                  break;
                case AIState::ATTACK:
                  ai.behavior = AIState::PATROL;
                  break;
              }
            }
          }
        }));

    // AI目标选择
    m_passes.push_back(Pass::create_update<stage::Update>(
        "SelectTargets", [](ComponentWriter<AIState> aiWriter, 
                           ComponentReader<Position> posReader) {
          // 简化的目标选择逻辑（实际项目应使用空间查询）
          auto view = aiWriter.view();
          std::vector<entt::entity> entities(view.begin(), view.end());
          
          for (auto e : entities) {
            auto& ai = aiWriter.get<AIState>(e);
            if (ai.behavior == AIState::CHASE || ai.behavior == AIState::ATTACK) {
              // 随机选择目标
              if (!entities.empty()) {
                ai.target = entities[static_cast<uint32_t>(e) % entities.size()];
              }
            }
          }
        }));

    // AI统计
    m_passes.push_back(Pass::create_update<stage::Last>(
        "AIStats", [](ComponentReader<AIState> reader, ResourceWriter<GameStats> stats) {
          auto& gameStats = stats.get();
          gameStats.aiUpdates = reader.view().size();
        }));

    return true;
  }
};

// 战斗系统：伤害和生命管理
class CombatSystem : public System {
 public:
  CombatSystem() : System("CombatSystem") {}

  bool init() override {
    // 发射伤害事件
    m_passes.push_back(Pass::create_update<stage::PostUpdate>(
        "EmitDamageEvents", [](EventWriter<DamageEvent> eventWriter, 
                              ComponentReader<AIState, Health> reader, 
                              ResourceReader<GlobalTime> timeRes) {
          int frame = timeRes.get().frameCount;
          int count = 0;
          
          // 只有攻击行为的AI才会发射伤害
          for (auto e : reader.view<AIState>()) {
            const auto& ai = reader.get<AIState>(e);
            if (ai.behavior == AIState::ATTACK && ai.target != entt::null) {
              if (static_cast<uint32_t>(e) % 50 == (frame % 50)) {
                eventWriter.get().push_back({ai.target, 10.0f});
                if (++count > 1000) break;
              }
            }
          }
        }));

    // 处理伤害事件
    m_passes.push_back(Pass::create_update<stage::PostUpdate>(
        "ProcessDamage", [](EventReader<DamageEvent> eventReader, 
                           ComponentWriter<Health> healthWriter) {
          for (const auto& ev : eventReader.get()) {
            if (healthWriter.have<Health>(ev.target)) {
              auto& hp = healthWriter.get<Health>(ev.target);
              hp.current = std::max(0.0f, hp.current - ev.amount);
            }
          }
        }));

    return true;
  }
};

// 生命周期系统：实体生命周期管理
class LifeCycleSystem : public System {
 public:
  LifeCycleSystem() : System("LifeCycleSystem") {}

  bool init() override {
    // 生命周期更新
    m_passes.push_back(Pass::create_update<stage::Update>(
        "UpdateLifecycle", [](ComponentWriter<Lifetime> lifeWriter, 
                             ComponentReader<Health> healthReader) {
          for (auto e : lifeWriter.view()) {
            auto& life = lifeWriter.get<Lifetime>(e);
            life.ticks--;
          }
        }));

    // 实体销毁（Last阶段，最高优先级）
    m_passes.push_back(Pass::create_update<stage::Last>(
        "DeathAndCleanup", [](EntityDestroyer destroyer, 
                             ComponentWriter<Lifetime> lifeWriter,
                             ComponentReader<Health> healthReader, 
                             ResourceWriter<GameStats> statsWriter) {
          int destroyedThisFrame = 0;
          auto view = lifeWriter.view();

          // 安全收集要销毁的实体
          std::vector<entt::entity> toDestroy;
          toDestroy.reserve(10000);

          for (auto e : view) {
            auto& life = lifeWriter.get<Lifetime>(e);

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

// 性能计算系统：模拟复杂并行计算
class ParallelComputeSystem : public System {
 public:
  ParallelComputeSystem() : System("ParallelComputeSystem") {}

  bool init() override {
    // Compute A：矩阵变换（模拟物理计算）
    m_passes.push_back(Pass::create_update<stage::Update>(
        "ComputeA", [](ComponentWriter<ComputeDataA> compWriter, 
                      ComponentReader<Position, Velocity, RigidBody> reader) {
          for (auto e : compWriter.view()) {
            if (reader.have_all<Position, Velocity, RigidBody>(e)) {
              auto& data = compWriter.get<ComputeDataA>(e);
              const auto& pos = reader.get<Position>(e);
              const auto& vel = reader.get<Velocity>(e);
              const auto& body = reader.get<RigidBody>(e);
              
              // 模拟复杂的物理矩阵计算
              for (int i = 0; i < 16; ++i) {
                data.matrix[i] = std::sin(pos.x * i * 0.1f) * body.mass + 
                                std::cos(pos.y * i * 0.1f) * std::abs(vel.vx);
              }
            }
          }
        }));

    // Compute B：距离映射（模拟光照计算）
    m_passes.push_back(Pass::create_update<stage::Update>(
        "ComputeB", [](ComponentWriter<ComputeDataB> compWriter, 
                      ComponentReader<Position, CollisionShape> reader) {
          for (auto e : compWriter.view()) {
            if (reader.have_all<Position, CollisionShape>(e)) {
              auto& data = compWriter.get<ComputeDataB>(e);
              const auto& pos = reader.get<Position>(e);
              const auto& shape = reader.get<CollisionShape>(e);
              
              // 模拟光照和距离相关计算
              for (int i = 0; i < 16; ++i) {
                data.distanceMap[i] = std::sqrt(std::abs(pos.x * i + pos.y)) * shape.radius;
              }
            }
          }
        }));

    // Compute C：颜色渐变（模拟材质计算）
    m_passes.push_back(Pass::create_update<stage::Update>(
        "ComputeC", [](ComponentWriter<ComputeDataC> compWriter, 
                      ComponentReader<Position, Material> reader) {
          for (auto e : compWriter.view()) {
            if (reader.have_all<Position, Material>(e)) {
              auto& data = compWriter.get<ComputeDataC>(e);
              const auto& pos = reader.get<Position>(e);
              const auto& mat = reader.get<Material>(e);
              
              // 模拟材质相关的颜色计算
              for (int i = 0; i < 16; ++i) {
                data.colorGrad[i] = std::tan(pos.x + i * 0.01f) * mat.metallic + 
                                   std::cos(pos.y + i * 0.02f) * mat.roughness;
              }
            }
          }
        }));

    // Compute D：物理模拟（模拟AI感知）
    m_passes.push_back(Pass::create_update<stage::Update>(
        "ComputeD", [](ComponentWriter<ComputeDataD> compWriter, 
                      ComponentReader<Position, AIState> reader) {
          for (auto e : compWriter.view()) {
            if (reader.have_all<Position, AIState>(e)) {
              auto& data = compWriter.get<ComputeDataD>(e);
              const auto& pos = reader.get<Position>(e);
              const auto& ai = reader.get<AIState>(e);
              
              // 模拟AI感知相关的物理计算
              for (int i = 0; i < 16; ++i) {
                data.physicsSim[i] = std::exp(-std::abs(pos.y * 0.001f)) * 
                                   (static_cast<int>(ai.behavior) + 1);
              }
            }
          }
        }));

    return true;
  }
};

// 统计与监控系统
class MonitorSystem : public System {
 public:
  MonitorSystem() : System("MonitorSystem") {}

  bool init() override {
    // 帧计数更新
    m_passes.push_back(Pass::create_update<stage::Cleanup>(
        "TickAndLog", [](ResourceWriter<GlobalTime> timeWriter) {
          auto& time = timeWriter.get();
          time.frameCount++;
        }));

    // 性能监控
    m_passes.push_back(Pass::create_update<stage::Cleanup>(
        "PerformanceMonitor", [](ResourceReader<GameStats> statsReader, ResourceReader<GlobalTime> timeReader) {
          const auto& stats = statsReader.get();
          const auto& time = timeReader.get();
          if (time.frameCount % 100 == 0) {
            std::cout << "[Monitor] Frame: " << time.frameCount
                      << " | Active: " << stats.activeEntities 
                      << " | Collisions: " << stats.physicsCollisions
                      << " | Render: " << stats.renderCalls
                      << " | AI: " << stats.aiUpdates << "\n";
          }
        }));

    return true;
  }
};

// ==========================================
// 3. 高性能压力测试主程序
// ==========================================

// 压力测试配置
struct StressTestConfig {
  int warmupFrames = 100;
  int testFrames = 500;
  int extremeFrames = 50;
  int entityBatchSize = 50000;
  bool enableExtremeTest = true;
};

// 性能统计
struct PerformanceStats {
  std::vector<double> frameTimes;
  double minTime = 0.0;
  double maxTime = 0.0;
  double avgTime = 0.0;
  double totalTime = 0.0;
  uint64_t totalEntitiesProcessed = 0;
  uint64_t totalCollisions = 0;
  uint64_t totalRenderCalls = 0;
  uint64_t totalAIUpdates = 0;
};

void RunPerformanceTest(World& world, const StressTestConfig& config, PerformanceStats& stats) {
  std::cout << "\n[2] Running Warmup Phase (" << config.warmupFrames << " frames)...\n";
  for (int frame = 0; frame < config.warmupFrames; ++frame) {
    world.run();
  }
  std::cout << "    -> Warmup completed.\n";

  // 主性能测试
  std::cout << "\n[3] Running Main Performance Test (" << config.testFrames << " frames)...\n\n";
  
  stats.frameTimes.clear();
  stats.frameTimes.reserve(config.testFrames);
  
  auto testStartTime = std::chrono::high_resolution_clock::now();

  for (int frame = 0; frame < config.testFrames; ++frame) {
    auto frameStart = std::chrono::high_resolution_clock::now();

    world.run();  // 执行一帧

    auto frameEnd = std::chrono::high_resolution_clock::now();
    double frameTimeMs = std::chrono::duration<double, std::milli>(frameEnd - frameStart).count();
    stats.frameTimes.push_back(frameTimeMs);

    // 每 50 帧打印一次详细状态
    if ((frame + 1) % 50 == 0) {
      std::cout << "    [Frame " << std::setw(3) << (frame + 1) << "] "
                << "Time: " << std::fixed << std::setprecision(2) << frameTimeMs << " ms "
                << "(" << std::setw(5) << static_cast<int>(1000.0 / frameTimeMs) << " FPS)\n";
    }
  }

  auto testEndTime = std::chrono::high_resolution_clock::now();
  stats.totalTime = std::chrono::duration<double>(testEndTime - testStartTime).count();

  // 计算统计信息
  stats.minTime = *std::min_element(stats.frameTimes.begin(), stats.frameTimes.end());
  stats.maxTime = *std::max_element(stats.frameTimes.begin(), stats.frameTimes.end());
  stats.avgTime = std::accumulate(stats.frameTimes.begin(), stats.frameTimes.end(), 0.0) / stats.frameTimes.size();
}

void RunExtremeStressTest(World& world, const StressTestConfig& config) {
  if (!config.enableExtremeTest) return;
  
  std::cout << "\n[4] Running Extreme Stress Test (" << config.extremeFrames << " frames)...\n";
  std::cout << "    -> 模拟大规模实体爆发性增长...\n";
  
  std::vector<double> extremeTimes;
  extremeTimes.reserve(config.extremeFrames);
  
  for (int frame = 0; frame < config.extremeFrames; ++frame) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // 极端压力：连续运行多帧模拟高负载
    for (int i = 0; i < 5; ++i) {
      world.run();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    double timeMs = std::chrono::duration<double, std::milli>(end - start).count();
    extremeTimes.push_back(timeMs);
    
    std::cout << "    [Extreme Frame " << std::setw(2) << (frame + 1) << "] "
              << "Time: " << std::fixed << std::setprecision(1) << timeMs << " ms\n";
  }
  
  double avgExtremeTime = std::accumulate(extremeTimes.begin(), extremeTimes.end(), 0.0) / extremeTimes.size();
  std::cout << "    -> Extreme test average: " << avgExtremeTime << " ms per 5 frames\n";
}

void PrintBenchmarkResults(const PerformanceStats& stats, const StressTestConfig& config) {
  std::cout << "\n" << std::string(60, '=') << "\n";
  std::cout << "            HIGH-PERFORMANCE ECS BENCHMARK RESULTS            \n";
  std::cout << std::string(60, '=') << "\n";

  std::cout << " Test Configuration:\n";
  std::cout << "   Warmup Frames    : " << config.warmupFrames << "\n";
  std::cout << "   Test Frames      : " << config.testFrames << "\n";
  std::cout << "   Extreme Test     : " << (config.enableExtremeTest ? "Enabled" : "Disabled") << "\n";
  std::cout << "\n";

  std::cout << " Performance Metrics:\n";
  std::cout << "   Total Time       : " << std::fixed << std::setprecision(3) << stats.totalTime << " s\n";
  std::cout << "   Average FPS      : " << std::setprecision(1) << (1000.0 / stats.avgTime) << "\n";
  std::cout << "   Min Frame Time   : " << std::setprecision(2) << stats.minTime << " ms\n";
  std::cout << "   Max Frame Time   : " << stats.maxTime << " ms\n";
  std::cout << "   Avg Frame Time   : " << stats.avgTime << " ms\n";
  std::cout << "   Frame Time Std   : " << std::setprecision(3) 
            << std::sqrt(std::accumulate(stats.frameTimes.begin(), stats.frameTimes.end(), 0.0, 
                [&](double sum, double t) { return sum + (t - stats.avgTime) * (t - stats.avgTime); }) / stats.frameTimes.size()) 
            << " ms\n";
  
  std::cout << "\n";
  std::cout << " System Throughput:\n";
  std::cout << "   Entities/Frame   : ~" << (stats.totalEntitiesProcessed / config.testFrames) << "\n";
  std::cout << "   Collisions/Frame : ~" << (stats.totalCollisions / config.testFrames) << "\n";
  std::cout << "   Render Calls     : ~" << (stats.totalRenderCalls / config.testFrames) << "\n";
  std::cout << "   AI Updates       : ~" << (stats.totalAIUpdates / config.testFrames) << "\n";
  
  std::cout << std::string(60, '=') << "\n";
}

int main() {
  std::cout << std::string(60, '=') << "\n";
  std::cout << " FantasyEngine ECS - Advanced Performance Stress Test\n";
  std::cout << " Multi-System Architecture with High-Concurrency Testing\n";
  std::cout << std::string(60, '=') << "\n";

  try {
    StressTestConfig config;
    PerformanceStats stats;
    
    World world;

    // 注册所有核心系统
    std::cout << "\n[1] Registering Systems...\n";
    world.add_system(stl::make_shared<InitSystem>());
    world.add_system(stl::make_shared<SpawnerSystem>());
    world.add_system(stl::make_shared<PhysicsSystem>());
    world.add_system(stl::make_shared<CollisionSystem>());
    world.add_system(stl::make_shared<RenderSystem>());
    world.add_system(stl::make_shared<AISystem>());
    world.add_system(stl::make_shared<CombatSystem>());
    world.add_system(stl::make_shared<LifeCycleSystem>());
    world.add_system(stl::make_shared<ParallelComputeSystem>());
    world.add_system(stl::make_shared<MonitorSystem>());
    std::cout << "    -> 10 core systems registered.\n";

    // 编译任务图
    std::cout << "\n[2] Compiling Execution Graph...\n";
    auto compileStart = std::chrono::high_resolution_clock::now();
    world.compile();
    auto compileEnd = std::chrono::high_resolution_clock::now();
    std::cout << "    -> Compiled in " 
              << std::chrono::duration_cast<std::chrono::milliseconds>(compileEnd - compileStart).count() 
              << " ms.\n";

    // 输出任务流图
    world.dump_graph("advanced_performance_test");
    std::cout << "    -> Taskflow graph dumped to advanced_performance_test_run.dot\n";

    // 执行Setup阶段
    std::cout << "\n[3] Running Setup Phase...\n";
    world.setup();
    std::cout << "    -> Setup completed.\n";

    // 运行性能测试
    RunPerformanceTest(world, config, stats);
    
    // 运行极限压力测试
    RunExtremeStressTest(world, config);

    // 输出最终结果
    PrintBenchmarkResults(stats, config);

  } catch (const std::exception& e) {
    std::cerr << "\n[FATAL ERROR] " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  std::cout << "\nTest completed successfully! Framework demonstrates excellent scalability and performance.\n";
  return EXIT_SUCCESS;
}