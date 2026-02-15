#pragma once

#include "Resource.h"
#include "ResourceCommandBuffer.h"

namespace re::engine::ecs {
class ResourceVisitor;

class RE_API ResourceManager {
 public:
  ResourceManager() = default;
  ~ResourceManager() = default;

  ResourceManager(const ResourceManager&) = delete;
  ResourceManager& operator=(const ResourceManager&) = delete;

  RE_FINLINE bool HasResource(ResourceId id) const {
    return (id.value < m_Resources.size() && id.version == m_ResourceIdVersions[id.value] && m_Resources[id.value].Valid());
  }
  RE_FINLINE const Resource* GetResource(ResourceId id) const {
    RE_ASSERT(HasResource(id));
    return &(m_Resources[id.value]);
  }

  ResourceId AddResource(Resource&& res);
  RE_FINLINE Resource* GetResource(ResourceId id) { return &(m_Resources[id.value]); }
  void RemoveResource(ResourceId id);

  template <typename T>
  void SetTypeResourceId(ResourceId id) {
    static const auto type_index = std::type_index(typeid(T));
    m_TypeResourceIdMap[type_index] = id;
  }

  template <typename T>
  ResourceId FindTypeResourceId() const {
    static const auto type_index = std::type_index(typeid(T));
    auto it = m_TypeResourceIdMap.find(type_index);
    if (it != m_TypeResourceIdMap.end()) {
      return it->second;
    }
    return ResourceId::Null;
  }

  RE_FINLINE void SetStringResourceId(const string& str, ResourceId id) { m_StringResourceIdMap[str] = id; }
  RE_FINLINE ResourceId FindStringResourceId(const string& str) const {
    auto it = m_StringResourceIdMap.find(str);
    if (it != m_StringResourceIdMap.end()) {
      return it->second;
    }
    return ResourceId::Null;
  }

  ResourceVisitor RequestResourceVisitor();

  void Submit(ResCommandBuffer&& buffer);
  void Flush();

 private:
  void ChangeResourceImpl(stl::pair<ResourceId, ResOperate>& changeRes);

  vector<Resource> m_Resources;
  vector<uint32_t> m_ResourceIdVersions;
  vector<ResourceId> m_FreeResourceIds;
  vector<ResCommandBuffer> m_CommandBuffers;
  std::mutex m_Mutex;

  unordered_map<std::type_index, ResourceId> m_TypeResourceIdMap;
  unordered_map<string, ResourceId> m_StringResourceIdMap;
};
}  // namespace re::engine::ecs