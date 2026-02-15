#pragma once

#include "ResourceCommandBuffer.h"
#include "ResourceManager.h"

namespace re::engine::ecs {
class RE_API ResourceVisitor {
 public:
  ResourceVisitor(ResourceManager* manager = nullptr) : m_ResourceManager(manager) {}

  RE_FINLINE void SetResourceManager(ResourceManager* manager) { m_ResourceManager = manager; }
  RE_FINLINE ResCommandBuffer* GetCommandBuffer() { return &m_ResCommandBuffer; }

  RE_FINLINE bool HasResource(ResId id) const {
    RE_ASSERT(m_ResourceManager != nullptr);
    return m_ResourceManager->HasResource(id);
  }
  RE_FINLINE const Resource* GetResource(ResId id) const {
    RE_ASSERT(m_ResourceManager != nullptr);
    return m_ResourceManager->GetResource(id);
  }

  template <typename T>
  ResId FindTypeResId() const {
    RE_ASSERT(m_ResourceManager != nullptr);
    return m_ResourceManager->GetTypeResId<T>();
  }

  RE_FINLINE ResId FindStringResId(const string& str) const {
    RE_ASSERT(m_ResourceManager != nullptr);
    return m_ResourceManager->FindStringResId(str);
  }

  RE_FINLINE void Submit() {
    RE_ASSERT(m_ResourceManager != nullptr);
    m_ResourceManager->Submit(std::move(m_ResCommandBuffer));
  }

 private:
  ResCommandBuffer m_ResCommandBuffer;
  ResourceManager* m_ResourceManager{nullptr};
};

}  // namespace re::engine::ecs