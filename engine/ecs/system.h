#pragma once

#include "pass/pass.h"

#include "worldBase.h"

namespace fe::engine::ecs {

class System {
 public:
  System() = default;
  virtual ~System() = default;

  System(const System&) = delete;
  System& operator=(const System&) = delete;

  FE_FINLINE bool IsAttach() const { return m_Attach; }
  FE_FINLINE bool IsEnabled() const { return m_Enabled; }

  virtual void OnAttach(World* world);
  virtual void OnDetach();

  virtual bool OnEnable() {
    if (!m_Enabled) {
      m_Enabled = true;
    }
    return m_Enabled;
  }
  virtual bool OnDisable() {
    if (m_Enabled) {
      m_Enabled = true;
    }
    return !m_Enabled;
  }

  virtual void OnPreUpdate() {}
  virtual void OnPostUpdate() {}

  FE_FINLINE vector<shared_ptr<SystemPass>> GetAllPass() { return m_Passes; }

 protected:
  template <typename ComponentType>
  bool PollAddComponentTemp();

  template <typename ComponentType>
  bool PollDelComponentTemp();

  template <typename ComponentType>
  bool PollChangeComponentTemp();

  World* m_World = nullptr;
  bool m_Enabled = false;
  bool m_Attach = false;

  vector<shared_ptr<SystemPass>> m_Passes;
};

}  // namespace fe::engine::ecs