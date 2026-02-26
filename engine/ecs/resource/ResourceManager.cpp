#include "resourceManager.h"

namespace fe::engine::ecs {
void ResourceManager::submit(ResourceCommandBuffer&& buffer) {
  m_commandBuffers.push_back(std::move(buffer));
}

ResourceId ResourceManager::addResource(Resource&& res) {
  ResourceId id;
  if (m_freeResourceIds.empty()) {
    id = m_resources.size();
    m_resources.push_back(std::move(res));
    m_resourceIdVersions.push_back(0);
  } else {
    id = m_freeResourceIds.back();
    m_freeResourceIds.pop_back();
    m_resources[id.value] = std::move(res);
    id.version = m_resourceIdVersions[id.value];
  }
  return id;
}
bool ResourceManager::removeResource(ResourceId id) {
  if (hasResource(id)) {
    m_freeResourceIds.push_back(id);
    m_resources[id.value].destroy();
    m_resourceIdVersions[id.value] += 1;
    return true;
  } else {
    return false;
  }
}

void ResourceManager::changeResourceImpl(stl::pair<ResourceId, ResourceOperate>& changeRes) {
  changeRes.second(m_resources[changeRes.first.value]);
}

void ResourceManager::flush() {
  uint32_t addResNum = 0;
  uint32_t removeResNum = 0;

  for (auto& cmb : m_commandBuffers) {
    addResNum += cmb.GetAddResources().size();
    removeResNum += cmb.GetRemoveResources().size();
  }

  int absAddResNum = addResNum - removeResNum;
  if (absAddResNum > 0) {
    m_resources.reserve(m_resources.size() + absAddResNum);
  }

  for (auto& cmb : m_commandBuffers) {
    auto& addResArray = cmb.GetAddResources();
    auto& changeResArray = cmb.GetChangeResources();
    auto& operateResArray = cmb.GetOperateResources();
    auto& removeResArray = cmb.GetRemoveResources();

    auto& opOrders = cmb.GetOpOrders();
    for (auto& op : opOrders) {
      if (op.first == ResourceCommandBuffer::OpType::Add) {
        *(addResArray[op.second].first) = addResource(std::move(addResArray[op.second].second));
      } else if (op.first == ResourceCommandBuffer::OpType::Operate) {
        changeResourceImpl(operateResArray[op.second]);
      } else if (op.first == ResourceCommandBuffer::OpType::Change) {
        m_resources[changeResArray[op.second].first.value] = std::move(changeResArray[op.second].second);
      } else if (op.first == ResourceCommandBuffer::OpType::Remove) {
        removeResource(removeResArray[op.second]);
      }
    }

    cmb.reset();
  }

  m_commandBuffers.clear();
}

}  // namespace fe::engine::ecs