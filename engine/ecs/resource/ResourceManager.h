#pragma once

#include "resource.h"
#include "resourceCommandBuffer.h"

namespace fe::engine::ecs {
class ResourceVisitor;

class ResourceManager {
 public:
  ResourceManager() = default;
  ~ResourceManager() = default;

  ResourceManager(const ResourceManager&) = delete;
  ResourceManager& operator=(const ResourceManager&) = delete;

  FE_FINLINE bool hasResource(ResourceId id) const {
    return (id.value < m_resources.size() && id.version == m_resourceIdVersions[id.value] && m_resources[id.value].valid());
  }

  FE_FINLINE Resource* getResource(ResourceId id) {
    FE_ASSERT(hasResource(id));
    return &(m_resources[id.value]);
  }
  FE_FINLINE const Resource* getResource(ResourceId id) const {
    FE_ASSERT(hasResource(id));
    return &(m_resources[id.value]);
  }

  ResourceId addResource(Resource&& res);
  bool removeResource(ResourceId id);

  template <typename T>
  void setTypeResourceId(ResourceId id) {
    static const auto type_index = std::type_index(typeid(T));
    m_typeResourceIdMap[type_index] = id;
  }

  template <typename T>
  ResourceId findTypeResourceId() const {
    static const auto type_index = std::type_index(typeid(T));
    auto it = m_typeResourceIdMap.find(type_index);
    if (it != m_typeResourceIdMap.end()) {
      return it->second;
    }
    return ResourceId();
  }

  FE_FINLINE void setStringResourceId(const stl::string& str, ResourceId id) { m_stringResourceIdMap[str] = id; }
  FE_FINLINE ResourceId findStringResourceId(const stl::string& str) const {
    auto it = m_stringResourceIdMap.find(str);
    if (it != m_stringResourceIdMap.end()) {
      return it->second;
    }
    return ResourceId();
  }

  void submit(ResourceCommandBuffer&& buffer);
  void flush();

 private:
  void changeResourceImpl(stl::pair<ResourceId, ResourceOperate>& changeRes);

  stl::vector<Resource> m_resources;
  stl::vector<uint32_t> m_resourceIdVersions;
  stl::vector<ResourceId> m_freeResourceIds;
  stl::vector<ResourceCommandBuffer> m_commandBuffers;

  stl::unordered_map<std::type_index, ResourceId> m_typeResourceIdMap;
  stl::unordered_map<stl::string, ResourceId> m_stringResourceIdMap;
};
}  // namespace fe::engine::ecs