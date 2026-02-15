#include "resCommandBuffer.h"

namespace fe::engine::ecs {
ResCommandBuffer::~ResCommandBuffer() {
  Reset();
}

void ResCommandBuffer::Reset() {
  for (auto& [promise, res] : m_AddResources) {
    memory::Allocator::destroy<ResIdPromise>(promise);
  }

  m_AddResources.clear();
  m_OperateResources.clear();
  m_ChangeResources.clear();
  m_RemoveResources.clear();
  m_Orders.clear();
}
}  // namespace fe::engine::ecs