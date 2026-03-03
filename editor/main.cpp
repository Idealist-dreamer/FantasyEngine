#include <iostream>
#include <chrono>
#include <thread>

#include "engine/engine.h"

using namespace fe::engine::ecs;
using namespace fe::engine;

// ---------------------------------------------------------
// 1. Define Components
// ---------------------------------------------------------
struct Transform {
  float x = 0.0f;
  float y = 0.0f;
};

struct Velocity {
  float vx = 0.0f;
  float vy = 0.0f;
};

struct Name {
  std::string value;
};

struct Health {
  int hp = 100;
};

// ---------------------------------------------------------
// 2. Define Resources
// ---------------------------------------------------------
struct TimeResource {
  float delta_time = 0.016f;
  float total_time = 0.0f;
  int frame_count = 0;
};

struct SharedCounterResource {
  int counter = 0;
};

// ---------------------------------------------------------
// 3. Define Events
// ---------------------------------------------------------
struct DamageEvent {
  entt::entity target;
  int amount;
};

struct SpawnEvent {
  float x, y;
};

// ---------------------------------------------------------
// 4. Implement Systems
// ---------------------------------------------------------

// A helper function to print thread IDs safely
void LogThread(const std::string& prefix) {
  std::ostringstream oss;
  oss << "[" << prefix << "] Executing on thread: " << std::this_thread::get_id() << "\n";
  std::cout << oss.str();
}

// System 1: Initialization & Spawning
class SpawnerSystem : public System {
 public:
  SpawnerSystem() : System("SpawnerSystem") {}

  bool init() override {
    // A startup pass to initialize resources and initial entities
    m_passes.push_back(Pass::create_start(
                           "InitWorld",
                           [](EntityCreator ec, ResourceWriter<TimeResource> time_res, ResourceWriter<SharedCounterResource> counter_res) {
                             std::cout << "[InitWorld] Setting up initial world state.\n";

                             // Init Resources
                             time_res.create();
                             counter_res.create();

                             // Create initial entities
                             for (int i = 0; i < 3; ++i) {
                               auto e = ec.create();
                               // We use registry directly here just for initial setup convenience,
                               // but strictly speaking we should use a command buffer or component writer
                             }
                           },
                           Priority::High)
                           .set_stage<stage::Startup>());

    // A pass that reads SpawnEvents and creates entities delayed
    m_passes.push_back(Pass::create_update("ProcessSpawns", [](EventReader<SpawnEvent> spawn_events, EntityCommandBuffer ecb) {
                         for (const auto& ev : spawn_events.get()) {
                           std::cout << "[ProcessSpawns] Spawning new entity at (" << ev.x << ", " << ev.y << ")\n";
                           uint32_t id = ecb.create();
                           // Note: To attach components via ECB, your EntityCommandBuffer needs an add() method.
                           // Since it doesn't currently, we just create the raw entity handle here.
                           // In a real scenario, ECB should record component additions.
                         }
                       }).set_stage<stage::Update>());

    return true;
  }
};

// System 2: Physics System (Demonstrates Concurrency)
class PhysicsSystem : public System {
 public:
  PhysicsSystem() : System("PhysicsSystem") {}

  bool init() override {
    // Pass A: Update positions based on velocity
    // Reads Velocity, Time; Writes Transform
    m_passes.push_back(Pass::create_update("MovementPass", [](ComponentWriter<Transform, Velocity> transforms,  // Writer acts as reader too
                                                              ResourceReader<TimeResource> time) {
                         LogThread("MovementPass");
                         float dt = time.get().delta_time;

                         auto view = transforms.view<Transform, const Velocity>();
                         for (auto [e, trans, vel] : view.each()) {
                           trans.x += vel.vx * dt;
                           trans.y += vel.vy * dt;
                         }

                         // Simulate heavy work to make concurrency visible
                         std::this_thread::sleep_for(std::chrono::milliseconds(50));
                       }).set_stage<stage::Update>());

    // Pass B: A completely independent pass that only reads Transforms
    // This SHOULD run concurrently with something else, but NOT MovementPass (because Movement writes Transform)
    m_passes.push_back(Pass::create_update("BoundaryCheckPass", [](ComponentReader<Transform> transforms) {
                         LogThread("BoundaryCheckPass");
                         auto view = transforms.view<const Transform>();
                         for (auto [e, trans] : view.each()) {
                           if (trans.x > 1000.0f) { /* do something */
                           }
                         }
                         std::this_thread::sleep_for(std::chrono::milliseconds(50));
                       }).set_stage<stage::PostUpdate>());  // Run after MovementPass finishes

    return true;
  }
};

// System 3: Logic System (Demonstrates Events and Concurrency)
class LogicSystem : public System {
 public:
  LogicSystem() : System("LogicSystem") {}

  bool init() override {
    // Pass C: Generates damage events randomly.
    // Writes Event<DamageEvent>
    m_passes.push_back(
        Pass::create_update("DamageGeneratorPass", [](EntityQuery eq, ComponentReader<Health> healths, EventWriter<DamageEvent> damage_writer) {
          LogThread("DamageGeneratorPass");

          // Just simulate generating an event for the first entity we find
          auto view = healths.view<const Health>();
          for (auto [e, h] : view.each()) {
            if (h.hp > 0) {
              damage_writer.get().push_back({e, 10});
              break;  // Just one per frame for test
            }
          }

          // Simulate work
          std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }).set_stage<stage::Update>());  // This CAN run concurrently with MovementPass!
                                         // (Different components/resources accessed)

    // Pass D: Processes damage events
    // Reads Event<DamageEvent>, Writes Component<Health>
    m_passes.push_back(Pass::create_update("DamageApplyPass",
                                           [](EventReader<DamageEvent> damage_reader, ComponentWriter<Health> healths) {
                                             LogThread("DamageApplyPass");
                                             const auto& events = damage_reader.get();
                                             for (const auto& ev : events) {
                                               if (healths.have<Health>(ev.target)) {
                                                 auto& hp = healths.get<Health>(ev.target).hp;
                                                 hp -= ev.amount;
                                                 std::cout << "[DamageApplyPass] Entity " << static_cast<uint32_t>(ev.target) << " took " << ev.amount
                                                           << " damage. HP now: " << hp << "\n";
                                               }
                                             }
                                           })
                           .set_stage<stage::Update>()
                           .set_after_stage<LogicSystem /*dummy*/>());
    // Note: Taskflow graph logic handles event read/write conflicts automatically!
    // Because DamageGenerator writes event, and DamageApply reads event,
    // the Mutex logic will automatically make DamageApply depend on DamageGenerator.

    return true;
  }
};

// System 4: Core Engine loop management
class CoreSystem : public System {
 public:
  CoreSystem() : System("CoreSystem") {}

  bool init() override {
    m_passes.push_back(Pass::create_update("TimeUpdatePass", [](ResourceWriter<TimeResource> time) {
                         time.get().frame_count++;
                         time.get().total_time += time.get().delta_time;
                         std::cout << "\n--- Frame " << time.get().frame_count << " ---\n";
                       }).set_stage<stage::PreUpdate>());

    return true;
  }
};

// ---------------------------------------------------------
// Main Execution
// ---------------------------------------------------------
int main() {
  std::cout << "==========================================\n";
  std::cout << " Testing ECS Framework (Taskflow Backend) \n";
  std::cout << "==========================================\n";

  // 1. Create the World
  World world;

  // 2. Register Systems
  world.add_system(stl::make_shared<CoreSystem>());
  world.add_system(stl::make_shared<SpawnerSystem>());
  world.add_system(stl::make_shared<PhysicsSystem>());
  world.add_system(stl::make_shared<LogicSystem>());

  // 3. Compile the Taskflow graph based on Pass dependencies and Mutexes
  std::cout << "Compiling World Graph...\n";
  world.compile();

  // Optional: Dump the graph to see the visual representation of dependencies
  world.dump_graph("graphs/ecs_graph");
  std::cout << "Graph dumped to graphs/ directory (use Graphviz/dot to view).\n\n";

  // 4. Run Setup (Start Passes)
  std::cout << "Running Setup...\n";
  world.setup();

  // 5. Inject some initial components manually for testing
  // In a real scenario, this is done via Initializer passes or Prefabs
  {
    // Get registry to inject test data
    // Note: this breaks pure encapsulation for testing purposes
    struct HackWorld : public WorldBase {
      entt::registry& get_reg() { return m_registry; }
    };
    auto& reg = reinterpret_cast<HackWorld&>(world).get_reg();

    auto e1 = reg.create();
    reg.emplace<Transform>(e1, 0.0f, 0.0f);
    reg.emplace<Velocity>(e1, 10.0f, 0.0f);
    reg.emplace<Health>(e1, 100);

    auto e2 = reg.create();
    reg.emplace<Transform>(e2, 100.0f, 100.0f);
    reg.emplace<Velocity>(e2, -5.0f, -5.0f);
    reg.emplace<Health>(e2, 50);
  }

  // 6. Run the Update loop (Update Passes)
  std::cout << "\nRunning Update Loop...\n";

  auto start_time = std::chrono::high_resolution_clock::now();

  const int NUM_FRAMES = 3;
  for (int i = 0; i < NUM_FRAMES; ++i) {
    // Run executes the taskflow graph for one frame and calls next_frame()
    world.run();

    // Wait a bit to simulate frame pacing
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> duration = end_time - start_time;

  std::cout << "\n==========================================\n";
  std::cout << " Simulation finished.\n";
  std::cout << " Total loop time for " << NUM_FRAMES << " frames: " << duration.count() << " ms\n";

  // Note: If running sequentially, 3 frames with 2x 50ms sleeps per frame = ~300ms minimum.
  // If Taskflow runs `MovementPass` and `DamageGeneratorPass` concurrently,
  // it will take ~150ms total for the heavy passes, proving multithreading works!

  if (duration.count() < 250.0) {
    std::cout << " MULTITHREADING SUCCESS: Passes executed concurrently!\n";
  } else {
    std::cout << " MULTITHREADING NOTE: Passes executed sequentially (Check Thread IDs above).\n";
  }
  std::cout << "==========================================\n";

  return 0;
}