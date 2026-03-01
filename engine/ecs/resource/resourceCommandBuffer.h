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
    m_orders.push_back({OpType::Add, m_add_resources.size()});
    m_add_resources.emplace_back(resIdPtr, std::move(res));
    return resIdPtr;
  }

  template <typename F>
  void operate_resource(ResourceId id, F&& op) {
    m_orders.push_back({OpType::Operate, m_operate_resources.size()});
    m_operate_resources.emplace_back(id, std::forward<F>(op));
  }

  void change_resource(ResourceId id, Resource&& res) {
    m_orders.push_back({OpType::Change, m_change_resources.size()});
    m_change_resources.emplace_back(id, std::move(res));
  }

  void remove_resource(ResourceId id) {
    m_orders.push_back({OpType::Remove, m_remove_resources.size()});
    m_remove_resources.push_back(id);
  }

  void reset();

 private:
  FE_FINLINE stl::vector<stl::pair<stl::shared_ptr<ResourceId>, Resource>>& get_add_resources() { return m_add_resources; }
  FE_FINLINE stl::vector<stl::pair<ResourceId, Resource>>& get_change_resources() { return m_change_resources; }
  FE_FINLINE stl::vector<stl::pair<ResourceId, ResourceOperate>>& get_operate_resources() { return m_operate_resources; }
  FE_FINLINE stl::vector<ResourceId>& get_remove_resources() { return m_remove_resources; }
  FE_FINLINE stl::vector<stl::pair<OpType, uint32_t>>& get_op_orders() { return m_orders; }

  stl::vector<stl::pair<stl::shared_ptr<ResourceId>, Resource>> m_add_resources;
  stl::vector<stl::pair<ResourceId, Resource>> m_change_resources;
  stl::vector<stl::pair<ResourceId, ResourceOperate>> m_operate_resources;
  stl::vector<ResourceId> m_remove_resources;
  stl::vector<stl::pair<OpType, uint32_t>> m_orders;
};
}  // namespace fe::engine::ecs