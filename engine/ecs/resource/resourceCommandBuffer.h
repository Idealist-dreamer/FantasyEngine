#pragma once

#include <functional>

#include "resource.h"

#include "engine/memory/allocator.h"
#include "engine/container/stl.h"
#include "engine/utility/class.h"
#include "engine/utility/assert.h"

namespace fe::engine::ecs {
struct ResourceId {
  static constexpr uint32_t Invalid = FE_UINT32_MAX;

  ResourceId(uint32_t _value = Invalid, uint32_t _version = 0) : value(_value), version(_version) {}

  bool null() const { return value == Invalid; }
  bool operator==(const ResourceId& other) const { return value == other.value && version == other.version; }

  uint32_t value;
  uint32_t version;
};

using ResOperate = std::function<void(Resource&)>;

class ResourceCommandBuffer : utility::NonCopyable {
  friend class ResourceManager;
  enum struct OpType : uint8_t { Add = 0, Change, Operate, Remove };

 public:
  ResourceCommandBuffer() = default;
  ~ResourceCommandBuffer();

  ResourceCommandBuffer(ResourceCommandBuffer&& other) noexcept = default;
  ResourceCommandBuffer& operator=(ResourceCommandBuffer&& other) noexcept = default;

  stl::shared_ptr<ResourceId> addResource(Resource&& res) {
    auto resIdPtr = stl::make_shared<ResourceId>();
    m_orders.push_back({OpType::Add, m_addResources.size()});
    m_addResources.emplace_back(resIdPtr, std::move(res));
    return resIdPtr;
  }

  template <typename F>
  void operateResource(ResourceId id, F&& op) {
    m_orders.push_back({OpType::Operate, m_operateResources.size()});
    m_operateResources.emplace_back(id, std::forward<F>(op));
  }

  void changeResource(ResourceId id, Resource&& res) {
    m_orders.push_back({OpType::Change, m_changeResources.size()});
    m_changeResources.emplace_back(id, std::move(res));
  }

  void removeResource(ResourceId id) {
    m_orders.push_back({OpType::Remove, m_removeResources.size()});
    m_removeResources.push_back(id);
  }

  void reset();

 private:
  FE_FINLINE stl::vector<stl::pair<stl::shared_ptr<ResourceId>, Resource>>& GetAddResources() { return m_addResources; }
  FE_FINLINE stl::vector<stl::pair<ResourceId, Resource>>& GetChangeResources() { return m_changeResources; }
  FE_FINLINE stl::vector<stl::pair<ResourceId, ResOperate>>& GetOperateResources() { return m_operateResources; }
  FE_FINLINE stl::vector<ResourceId>& GetRemoveResources() { return m_removeResources; }
  FE_FINLINE stl::vector<stl::pair<OpType, uint32_t>>& GetOpOrders() { return m_orders; }

  stl::vector<stl::pair<stl::shared_ptr<ResourceId>, Resource>> m_addResources;
  stl::vector<stl::pair<ResourceId, Resource>> m_changeResources;
  stl::vector<stl::pair<ResourceId, ResOperate>> m_operateResources;
  stl::vector<ResourceId> m_removeResources;
  stl::vector<stl::pair<OpType, uint32_t>> m_orders;
};
}  // namespace fe::engine::ecs