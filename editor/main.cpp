#include <iostream>

#include "engine/engine.h"

using namespace fe::engine::ecs;
using namespace fe::engine;

// 速度组件
struct Velocity {
  glm::vec3 value;
};

// 全局配置资源
struct GlobalSettings {
  float timeScale = 1.0f;
};

// 物理系统：根据速度更新位置
class PhysicsSystem : public System {
 public:
  void onInit() override {
    // 创建 Pass 并声明权限
    // 需要：写 Transform, 读 Velocity, 读 ResourceManager(获取全局设置)
    createPass<WriteComponent<Transform>, ReadComponent<Velocity>, ReadClass<ResourceManager>>("PhysicsPass", [](auto& query) {
      // 获取 DeltaTime (模拟逻辑)
      float dt = 0.016f;

      // 尝试从资源管理器获取全局缩放
      ResourceId configId = query.findTypeResourceId<GlobalSettings>();
      if (!configId.null()) {
        dt *= query.getResourceConst(configId)->template get<GlobalSettings>()->timeScale;
      }

      // 获取 View 并迭代
      auto view_trans = query.template view<Transform>();
      for (Entity entity : view_trans) {
        auto& trans = query.template getComponent<Transform>(entity);
        const auto& vel = query.template getComponentConst<Velocity>(entity);

        trans.position += vel.value * dt;
      }

      std::cout << "[PhysicsSystem] Updated Entities. DT: " << dt << std::endl;
    });
  }
};

// 渲染/打印系统：只读位置并打印
class LogSystem : public System {
 public:
  void onInit() override {
    // 声明权限：只读 Transform
    createPass<WriteComponent<Transform>>("LogPass", [](auto& query) {
      auto view = query.template viewConst<Transform>();
      int count = 0;
      for (Entity entity : view) {
        const auto& trans = query.template getComponent<Transform>(entity);
        std::cout << "  Entity[" << (uint32_t)entity << "] Pos: (" << trans.position.x << ", " << trans.position.y << ", " << trans.position.z << ")"
                  << std::endl;
        count++;
      }
      std::cout << "[LogSystem] Displayed " << count << " entities." << std::endl;
    });
  }
};

// ---------------------------------------------------------
// 4. Main 函数
// ---------------------------------------------------------

int main() {
  std::cout << "Hello FantasyEngine ECS!" << std::endl;

  // 1. 创建世界
  World world;

  // 2. 添加全局资源
  {
    auto settings = Resource::create<GlobalSettings>();
    settings.get<GlobalSettings>()->timeScale = 2.0f;  // 2倍速运行

    ResourceId resId = world.resourceManager()->addResource(std::move(settings));
    world.resourceManager()->setTypeResourceId<GlobalSettings>(resId);
  }

  // 3. 注册系统
  // 注意：World 会在第一次 run 时自动编译 Scheduler
  world.addSystem(stl::string("Physics"), stl::make_shared<PhysicsSystem>());
  world.addSystem(stl::string("Logger"), stl::make_shared<LogSystem>());

  // 4. 创建一些测试实体
  {
    // 实体 A
    Entity a = world.createEntity();
    // Transform 组件在 createEntity 时已默认添加（根据你的 WorldBase::createEntity 实现）
    world.addComponent<Velocity>(a, glm::vec3(1.0f, 0.0f, 0.0f));

    // 实体 B
    Entity b = world.createEntity();
    world.getComponent<Transform>(b).position = glm::vec3(0.0f, 10.0f, 0.0f);
    world.addComponent<Velocity>(b, glm::vec3(0.0f, -1.0f, 0.0f));
  }

  // 5. 运行循环
  std::cout << "--- Starting Simulation ---" << std::endl;
  for (int frame = 0; frame > -5; ++frame) {
    std::cout << "\nFrame: " << frame << std::endl;
    world.run();
  }

  std::cout << "\nSimulation Finished!" << std::endl;

  return 0;
}