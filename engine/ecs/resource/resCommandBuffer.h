#pragma once

#include <functional>

#include "resource.h"

#include "engine/memory/allocator.h"
#include "engine/container/stl.h"

namespace fe::engine::ecs {
struct ResId {
  static constexpr uint32_t Invalid = FE_UINT32_MAX;

  ResId(uint32_t _value = Invalid, uint32_t _version = 0) : value(_value), version(_version) {}

  bool IsNull() const { return value == Invalid; }
  bool operator==(const ResId& other) const { return value == other.value && version == other.version; }

  uint32_t value;
  uint32_t version;
};

class ResIdPromise {
  friend class ResourceManager;

 public:
  bool IsReady() const { return !(id.IsNull()); }

  ResId GetId() const {
    if (id.IsNull()) {
      isGet = true;
      return id;
    }
    return id;
  }

 private:
  ResId id;
  mutable bool isGet = false;
};

using ResOperate = std::function<void(Resource&)>;

class ResCommandBuffer {
  friend class ResourceManager;
  enum struct OpType : uint8_t { Add = 0, Change, Operate, Remove };

 public:
  ResCommandBuffer() = default;
  ~ResCommandBuffer();

  ResCommandBuffer(const ResCommandBuffer&) = delete;
  ResCommandBuffer& operator=(const ResCommandBuffer&) = delete;

  ResCommandBuffer(ResCommandBuffer&& other) noexcept = default;
  ResCommandBuffer& operator=(ResCommandBuffer&& other) noexcept = default;

  ResIdPromise* AddResource(Resource&& res) {
    auto promise = memory::Allocator::create<ResIdPromise>();
    m_Orders.push_back({OpType::Add, m_AddResources.size()});
    m_AddResources.emplace_back(promise, std::move(res));
    return promise;
  }

  template <typename F>
  void OperateResource(ResId id, F&& op) {
    m_Orders.push_back({OpType::Operate, m_OperateResources.size()});
    m_OperateResources.emplace_back(id, std::forward<F>(op));
  }

  void ChangeResource(ResId id, Resource&& res) {
    m_Orders.push_back({OpType::Change, m_ChangeResources.size()});
    m_ChangeResources.emplace_back(id, std::move(res));
  }

  void RemoveResource(ResId id) {
    m_Orders.push_back({OpType::Remove, m_RemoveResources.size()});
    m_RemoveResources.push_back(id);
  }

  void Reset();

 private:
  FE_FINLINE stl::vector<stl::pair<ResIdPromise*, Resource>>& GetAddResources() { return m_AddResources; }
  FE_FINLINE stl::vector<stl::pair<ResId, Resource>>& GetChangeResources() { return m_ChangeResources; }
  FE_FINLINE stl::vector<stl::pair<ResId, ResOperate>>& GetOperateResources() { return m_OperateResources; }
  FE_FINLINE stl::vector<ResId>& GetRemoveResources() { return m_RemoveResources; }
  FE_FINLINE stl::vector<stl::pair<OpType, uint32_t>>& GetOpOrders() { return m_Orders; }

  stl::vector<stl::pair<ResIdPromise*, Resource>> m_AddResources;
  stl::vector<stl::pair<ResId, Resource>> m_ChangeResources;
  stl::vector<stl::pair<ResId, ResOperate>> m_OperateResources;
  stl::vector<ResId> m_RemoveResources;
  stl::vector<stl::pair<OpType, uint32_t>> m_Orders;
};
}  // namespace fe::engine::ecs