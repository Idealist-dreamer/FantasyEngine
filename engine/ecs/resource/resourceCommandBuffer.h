#pragma once

#include <functional>

#include "resource.h"

#include "engine/base/memory/allocator.h"
#include "engine/base/container/stl.h"
#include "engine/base/utility/assert.h"

namespace fe::engine::ecs {
using ResourceOperate = std::function<void(Resource&)>;

class ResourceCommandBuffer {
  friend class ResourceManager;
  enum struct OpType : uint8_t { Add = 0, Change, Operate, Remove };

 public:
  ResourceCommandBuffer() = default;
  ~ResourceCommandBuffer();

  ResourceCommandBuffer(const ResourceCommandBuffer&) = delete;
  ResourceCommandBuffer& operator=(const ResourceCommandBuffer&) = delete;

  ResourceCommandBuffer(ResourceCommandBuffer&&) noexcept = default;
  ResourceCommandBuffer& operator=(ResourceCommandBuffer&&) noexcept = default;

  stl::shared_ptr<ResourceId> add_resource(Resource&& res) {
    auto resIdPtr = stl::make_shared<ResourceId>();
    m_orders.push_back({OpType::Add, m_addResources.size()});
    m_addResources.emplace_back(resIdPtr, std::move(res));
    return resIdPtr;
  }

  template <typename F>
  void operate_resource(ResourceId id, F&& op) {
    m_orders.push_back({OpType::Operate, m_operateResources.size()});
    m_operateResources.emplace_back(id, std::forward<F>(op));
  }

  void change_resource(ResourceId id, Resource&& res) {
    m_orders.push_back({OpType::Change, m_changeResources.size()});
    m_changeResources.emplace_back(id, std::move(res));
  }

  void remove_resource(ResourceId id) {
    m_orders.push_back({OpType::Remove, m_removeResources.size()});
    m_removeResources.push_back(id);
  }

  void reset();

 private:
  FE_FINLINE stl::vector<stl::pair<stl::shared_ptr<ResourceId>, Resource>>& get_add_resources() { return m_addResources; }
  FE_FINLINE stl::vector<stl::pair<ResourceId, Resource>>& get_change_resources() { return m_changeResources; }
  FE_FINLINE stl::vector<stl::pair<ResourceId, ResourceOperate>>& get_operate_resources() { return m_operateResources; }
  FE_FINLINE stl::vector<ResourceId>& get_remove_resources() { return m_removeResources; }
  FE_FINLINE stl::vector<stl::pair<OpType, uint32_t>>& get_op_orders() { return m_orders; }

  stl::vector<stl::pair<stl::shared_ptr<ResourceId>, Resource>> m_addResources;
  stl::vector<stl::pair<ResourceId, Resource>> m_changeResources;
  stl::vector<stl::pair<ResourceId, ResourceOperate>> m_operateResources;
  stl::vector<ResourceId> m_removeResources;
  stl::vector<stl::pair<OpType, uint32_t>> m_orders;
};
}  // namespace fe::engine::ecs