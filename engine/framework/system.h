#pragma once

#include "pass.h"
#include "serialization.h"

#include <typeindex>

namespace fe::engine {

class System {
 public:
  System(const stl::string& name) : m_name(name) {}
  virtual ~System() = default;

  virtual bool init(SceneBase& scene) = 0;

  virtual void save(SceneBase& scene, Archive&) {}
  virtual void load(SceneBase& scene, Archive&) {}

 protected:
  stl::string m_name;
  stl::vector<Pass> m_passes;

  friend class Scene;
};

}  // namespace fe::engine