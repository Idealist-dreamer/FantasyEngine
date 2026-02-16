#include "resourceCommandBuffer.h"

namespace fe::engine::ecs {
ResourceCommandBuffer::~ResourceCommandBuffer() {
  reset();
}

void ResourceCommandBuffer::reset() {
  m_addResources.clear();
  m_operateResources.clear();
  m_changeResources.clear();
  m_removeResources.clear();
  m_orders.clear();
}
}  // namespace fe::engine::ecs