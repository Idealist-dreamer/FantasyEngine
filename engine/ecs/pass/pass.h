#pragma once

#include "passInfo.h"

namespace fe::engine::ecs {
class WorldBase;

class Pass {
 public:
  Pass(const stl::string& name = "") : m_id(PassId::Create()), m_name(name) {}

  virtual ~Pass() = default;

  FE_FINLINE PassId id() const { return m_id; }
  FE_FINLINE const stl::string& name() const { return m_name; }

  FE_FINLINE void addPredecessor(PassId id) { return m_predecessors.push_back(id); }
  FE_FINLINE const stl::vector<PassId>& predecessors() const { return m_predecessors; }

  FE_FINLINE void addMutex(PassMutex mutex) { return m_mutexs.push_back(mutex); }
  FE_FINLINE const stl::vector<PassMutex>& mutexs() const { return m_mutexs; }

  virtual void execute() = 0;

 protected:
  PassId m_id;
  stl::string m_name;

  stl::vector<PassId> m_predecessors;
  stl::vector<PassMutex> m_mutexs;
};
}  // namespace fe::engine::ecs