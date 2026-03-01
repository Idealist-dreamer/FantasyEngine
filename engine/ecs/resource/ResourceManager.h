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

  FE_FINLINE bool has_resource(ResourceId id) const {
    return (id.m_value < m_resources.size() && id.m_version == m_id_versions[id.m_value] && m_resources[id.m_value].valid());
  }

  FE_FINLINE Resource* get_resource(ResourceId id) {
    FE_ASSERT(has_resource(id));
    return &(m_resources[id.m_value]);
  }
  FE_FINLINE const Resource* get_resource(ResourceId id) const {
    FE_ASSERT(has_resource(id));
    return &(m_resources[id.m_value]);
  }

  ResourceId add_resource(Resource&& res);
  bool remove_resource(ResourceId id);

  template <typename T>
  void set_resource_id(ResourceId id) {
    static const auto type_index = std::type_index(typeid(T));
    m_type_id_map[type_index] = id;
  }

  template <typename T>
  ResourceId find_resource_id() const {
    static const auto type_index = std::type_index(typeid(T));
    auto it = m_type_id_map.find(type_index);
    if (it != m_type_id_map.end()) {
      return it->second;
    }
    return ResourceId();
  }

  FE_FINLINE void set_resource_id(const stl::string& str, ResourceId id) { m_string_id_map[str] = id; }
  FE_FINLINE ResourceId find_resource_id(const stl::string& str) const {
    auto it = m_string_id_map.find(str);
    if (it != m_string_id_map.end()) {
      return it->second;
    }
    return ResourceId();
  }

  void submit(ResourceCommandBuffer&& buffer);
  void flush();

 private:
  void change_resource_impl(stl::pair<ResourceId, ResourceOperate>& changeRes);

  stl::vector<Resource> m_resources;
  stl::vector<uint32_t> m_id_versions;
  stl::vector<ResourceId> m_free_ids;
  stl::vector<ResourceCommandBuffer> m_command_buffers;

  stl::unordered_map<std::type_index, ResourceId> m_type_id_map;
  stl::unordered_map<stl::string, ResourceId> m_string_id_map;
};
}  // namespace fe::engine::ecs