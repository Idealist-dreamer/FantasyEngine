#pragma once

#include "resourceCommandBuffer.h"
#include "resourceManager.h"

namespace fe::engine::ecs {
class ResourceVisitor {
 public:
  ResourceVisitor(ResourceManager* manager = nullptr) : m_resourceManager(manager) {}

  FE_FINLINE void setResourceManager(ResourceManager* manager) { m_resCommandBuffer = manager; }
  FE_FINLINE ResourceCommandBuffer* getCommandBuffer() { return &m_resCommandBuffer; }

  FE_FINLINE bool HasResource(ResId id) const {
    FE_ASSERT(m_ResourceManager != nullptr);
    return m_ResourceManager->HasResource(id);
  }
  FE_FINLINE const Resource* GetResource(ResId id) const {
    FE_ASSERT(m_ResourceManager != nullptr);
    return m_ResourceManager->GetResource(id);
  }

  template <typename T>
  ResId FindTypeResId() const {
    FE_ASSERT(m_ResourceManager != nullptr);
    return m_ResourceManager->GetTypeResId<T>();
  }

  FE_FINLINE ResId FindStringResId(const string& str) const {
    FE_ASSERT(m_ResourceManager != nullptr);
    return m_ResourceManager->FindStringResId(str);
  }

  FE_FINLINE void Submit() {
    FE_ASSERT(m_ResourceManager != nullptr);
    m_ResourceManager->Submit(std::move(m_ResCommandBuffer));
  }

 private:
  ResourceCommandBuffer m_resCommandBuffer;
  ResourceManager* m_resourceManager{nullptr};
};

}  // namespace fe::engine::ecs