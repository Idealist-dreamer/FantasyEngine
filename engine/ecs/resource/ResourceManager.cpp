#include "resourceManager.h"

namespace fe::engine::ecs {
void ResourceManager::submit(ResourceCommandBuffer&& buffer) {
  m_commandBuffers.push_back(std::move(buffer));
}

ResourceId ResourceManager::add_resource(Resource&& res) {
  ResourceId id;
  if (m_freeResourceIds.empty()) {
    id = m_resources.size();
    m_resources.push_back(std::move(res));
    m_resourceIdVersions.push_back(0);
  } else {
    id = m_freeResourceIds.back();
    m_freeResourceIds.pop_back();
    m_resources[id.m_value] = std::move(res);
    id.m_version = m_resourceIdVersions[id.m_value];
  }
  return id;
}
bool ResourceManager::remove_resource(ResourceId id) {
  if (has_resource(id)) {
    m_freeResourceIds.push_back(id);
    m_resources[id.m_value].destroy();
    m_resourceIdVersions[id.m_value] += 1;
    return true;
  } else {
    return false;
  }
}

void ResourceManager::change_resource_impl(stl::pair<ResourceId, ResourceOperate>& changeRes) {
  changeRes.second(m_resources[changeRes.first.m_value]);
}

void ResourceManager::flush() {
  uint32_t addResNum = 0;
  uint32_t removeResNum = 0;

  for (auto& cmb : m_commandBuffers) {
    addResNum += cmb.get_add_resources().size();
    removeResNum += cmb.get_remove_resources().size();
  }

  int absAddResNum = addResNum - removeResNum;
  if (absAddResNum > 0) {
    m_resources.reserve(m_resources.size() + absAddResNum);
  }

  for (auto& cmb : m_commandBuffers) {
    auto& addResArray = cmb.get_add_resources();
    auto& changeResArray = cmb.get_change_resources();
    auto& operateResArray = cmb.get_operate_resources();
    auto& removeResArray = cmb.get_remove_resources();

    auto& opOrders = cmb.get_op_orders();
    for (auto& op : opOrders) {
      if (op.first == ResourceCommandBuffer::OpType::Add) {
        *(addResArray[op.second].first) = add_resource(std::move(addResArray[op.second].second));
      } else if (op.first == ResourceCommandBuffer::OpType::Operate) {
        change_resource_impl(operateResArray[op.second]);
      } else if (op.first == ResourceCommandBuffer::OpType::Change) {
        m_resources[changeResArray[op.second].first.m_value] = std::move(changeResArray[op.second].second);
      } else if (op.first == ResourceCommandBuffer::OpType::Remove) {
        remove_resource(removeResArray[op.second]);
      }
    }

    cmb.reset();
  }

  m_commandBuffers.clear();
}

}  // namespace fe::engine::ecs