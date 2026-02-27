#include <iostream>

#include "engine/engine.h"

using namespace fe::engine::ecs;
using namespace fe::engine;

// 1. 定义组件
struct Position {
  glm::vec3 val;
};
struct Velocity {
  glm::vec3 val;
};

// 2. 定义系统逻辑 (Pass 函数)
// 模拟物理系统：读取 Velocity，写入 Position
void PhysicsSystem(ComponentReader<Velocity> rd, ComponentWriter<Position> wr) {
  auto view = rd.view();  // 实际上这里需要处理 view 的交集，此处简写
  for (auto entity : view) {
    if (wr.all_of<Position>(entity)) {
      wr.get<Position>(entity).val += rd.get<Velocity>(entity).val;
      std::cout << "Entity updated pos\n";
    }
  }
}

// 模拟初始化系统：只运行一次
void InitSystem(EntityCreator ec, ComponentWriter<Position, Velocity> wr) {
  for (int i = 0; i < 10; ++i) {
    auto e = ec.create();
    wr.add<Position>(e, glm::vec3(0.0f));
    wr.add<Velocity>(e, glm::vec3(1.0f));
  }
  std::cout << "Created 10 entities\n";
}

// 3. 定义一个 System 类封装 Pass
class MovementSystem : public System {
 public:
  MovementSystem() : System("MovementSystem") {}
  void init(WorldBase& world) override {
    Pass p("PhysicsPass");
    p.init(PhysicsSystem);  // 自动推导 Mutex
    m_passes.push_back(p);
  }
};

class SetupSystem : public System {
 public:
  SetupSystem() : System("SetupSystem") {}
  void init(WorldBase& world) override {
    Pass p("InitPass");
    p.init(InitSystem);
    m_passes.push_back(p);
  }
};

int main() {
  World world;

  // 注册系统
  auto setup = stl::make_shared<SetupSystem>();
  auto move = stl::make_shared<MovementSystem>();

  world.addSystem(setup);
  world.addSystem(move);

  // 编译 DAG 图 (根据 Mutex 自动排布 InitPass -> PhysicsPass)
  std::cout << "Compiling Graph..." << std::endl;
  world.compile();

  // 导出图查看结构 (可选)
  // world.dumpGraph("task_graph.dot");

  std::cout << "--- Starting Simulation ---" << std::endl;
  for (int frame = 0; frame < 10; ++frame) {
    std::cout << "\nFrame: " << frame << std::endl;
    world.run();
    // 建议在 world.run 内部或之后显式调用资源 flush
    // world.getResourceManager().flush();
  }

  std::cout << "\nSimulation Finished!" << std::endl;
  return 0;
}