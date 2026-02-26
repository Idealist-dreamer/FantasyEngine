#include "worldBase.h"

namespace fe::engine::ecs {
WorldBase::WorldBase() {
  m_resourceManager = stl::make_unique<ResourceManager>();
}
WorldBase::~WorldBase() {}

}  // namespace fe::engine::ecs