#include "resourceCommandBuffer.h"

namespace fe::engine::ecs {
ResourceCommandBuffer::~ResourceCommandBuffer() {
  reset();
}

void ResourceCommandBuffer::reset() {
  m_add_resources.clear();
  m_operate_resources.clear();
  m_change_resources.clear();
  m_remove_resources.clear();
  m_orders.clear();
}
}  // namespace fe::engine::ecs