#pragma once

#include "common.h"

#include "access/param.h"
#include "resource/resourceManager.h"

namespace fe::engine::ecs {
class WorldBase {
 public:
  WorldBase() = default;
  virtual ~WorldBase() = default;

  template <typename EQ, typename std::enable_if_t<is_entity_query<EQ>::value, bool> = true>
  EQ getSuper() {
    return EQ(m_registry);
  }

  template <typename EC, typename std::enable_if_t<is_entity_creator<EC>::value, bool> = true>
  EC getSuper() {
    return EC(m_registry);
  }

  template <typename ED, typename std::enable_if_t<is_entity_destroyer<ED>::value, bool> = true>
  ED getSuper() {
    return ED(m_registry);
  }

  template <typename CR, std::enable_if_t<is_component_reader<CR>::value, bool> = true>
  CR getSuper() {
    return CR(m_registry);
  }

  template <typename CW, std::enable_if_t<is_component_writer<CW>::value, bool> = true>
  CW getSuper() {
    return CW(m_registry);
  }

  template <typename R, std::enable_if_t<is_resource_manager<R>::value, bool> = true>
  R getSuper() {
    using PlainType = std::remove_reference_t<R>;

    if constexpr (std::is_const_v<PlainType>) {
      return m_resourceManager;
    } else {
      return m_resourceManager;
    }
  }

 protected:
  Registry m_registry;
  ResourceManager m_resourceManager;
};
}  // namespace fe::engine::ecs