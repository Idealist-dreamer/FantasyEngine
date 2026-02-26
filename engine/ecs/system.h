#pragma once

#include "pass/pass.h"

#include "worldBase.h"
#include "query.h"

namespace fe::engine::ecs {
template <typename Q>
class GenericSystemPass : public Pass {
 public:
  using QueryType = Q;

  GenericSystemPass(const stl::string& name, WorldBase* world, std::function<void(Q&)> func) : Pass(name), m_func(func) {
    m_query.setWorld(world);
    this->m_mutexs = Q::getDependencies();
  }

  void execute() override {
    if (m_func) {
      m_func(m_query);
    }
  }

 private:
  Q m_query;
  std::function<void(Q&)> m_func;
};

class System {
 public:
  System() = default;
  virtual ~System() = default;

  System(const System&) = delete;

  void attach(WorldBase* world) {
    m_world = world;
    onInit();
  }

  virtual void onInit() = 0;

  const stl::vector<stl::shared_ptr<Pass>>& getPasses() const { return m_passes; }

 protected:
  template <typename... Tags, typename Func>
  stl::shared_ptr<Pass> createPass(const stl::string& name, Func&& func) {
    using Q = Query<Tags...>;
    auto pass = stl::make_shared<GenericSystemPass<Q>>(name, m_world, std::forward<Func>(func));
    m_passes.push_back(pass);
    return pass;
  }

  WorldBase* m_world = nullptr;
  stl::vector<stl::shared_ptr<Pass>> m_passes;
};

}  // namespace fe::engine::ecs