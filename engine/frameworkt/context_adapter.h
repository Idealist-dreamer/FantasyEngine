#pragma once

#include "scene_context.h"
#include "context_mutex.h"

#include "foundation/utility/meta.h"

namespace fe::engine {

template <typename T>
struct ContextAdapter {
  using CleanT = meta::clean_t<T>;

  static void prepare(SceneContext&, uint32_t pass_id) {}
  static ContextMutex get_mutex() { return ContextMutex{}; }
  static CleanT bind(SceneContext& sc, uint32_t pass_id) { return CleanT{}; }
};

struct ContextOps {
  template <typename... Args>
  static ContextMutex collect_mutexes() {
    ContextMutex merged;
    (merged.merge(ContextAdapter<Args>::get_mutex()), ...);
    return merged;
  }

  template <typename... Args>
  static void prepare_all(SceneContext& sc, uint32_t pass_id) {
    (ContextAdapter<Args>::prepare(sc, pass_id), ...);
  }

  template <typename... Args>
  static auto bind_all(SceneContext& sc, uint32_t pass_id) {
    return std::make_tuple(ContextAdapter<Args>::bind(sc, pass_id)...);
  }
};

}  // namespace fe::engine