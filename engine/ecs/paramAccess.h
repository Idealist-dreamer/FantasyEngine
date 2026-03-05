#pragma once

#include "paramTypes.h"
#include "paramTraits.h"
#include "meta.h"

namespace fe::engine::ecs {

// ============================================================================
// Parameter accessor
// Centralizes all parameter fetch logic, distinguishes pass-by-value vs pass-by-reference
// ============================================================================

struct ParamAccess {
  // EntityQuery: pass by value
  template <IsEntityQuery EQ>
  static auto get(Registry& reg) {
    return meta::clean_t<EQ>(reg);
  }

  // EntityCreator: pass by value
  template <IsEntityCreator EC>
  static auto get(Registry& reg) {
    return meta::clean_t<EC>(reg);
  }

  // EntityDestroyer: pass by value
  template <IsEntityDestroyer ED>
  static auto get(Registry& reg) {
    return meta::clean_t<ED>(reg);
  }

  // EntityCommandBuffer: pass by reference
  template <IsEntityCommandBuffer EB>
  static decltype(auto) get(stl::unordered_map<uint32_t, EntityCommandBuffer>& buffers, uint32_t passId) {
    return (buffers[passId]);  // return lvalue reference
  }

  // ComponentReader: pass by value
  template <IsComponentReader CR>
  static auto get(Registry& reg) {
    return meta::clean_t<CR>(reg);
  }

  // ComponentWriter: pass by value
  template <IsComponentWriter CW>
  static auto get(Registry& reg) {
    return meta::clean_t<CW>(reg);
  }

  // EventReader: pass by value
  template <IsEventReader ER>
  static auto get(const stl::unordered_map<std::type_index, ContextStorage>& eventMgr) {
    using T = typename is_event_reader<meta::clean_t<ER>>::type;
    auto it = eventMgr.find(std::type_index(typeid(T)));
    FE_ASSERT(it != eventMgr.end() && "Event type not registered!");
    return meta::clean_t<ER>(*it->second.template get<T>());
  }

  // EventWriter: pass by value
  template <IsEventWriter EW>
  static auto get(const stl::unordered_map<std::type_index, ContextStorage>& eventMgr) {
    using T = typename is_event_writer<meta::clean_t<EW>>::type;
    auto it = eventMgr.find(std::type_index(typeid(T)));
    FE_ASSERT(it != eventMgr.end() && "Event type not registered!");
    return meta::clean_t<EW>(*it->second.template get<T>());
  }

  // Context (Resource): pass by value
  template <IsContextParam R>
  static auto get(stl::unordered_map<std::type_index, ContextStorage>& resMgr) {
    using RawT = meta::clean_t<R>;
    using U = typename RawT::type;
    auto it = resMgr.find(std::type_index(typeid(U)));
    FE_ASSERT(it != resMgr.end() && "Context not registered!");
    return RawT(it->second);
  }
};

}  // namespace fe::engine::ecs
