#pragma once

#include <typeindex>
#include <functional>

#include "foundation/container/stl.h"
#include "foundation/utility/any.h"

namespace fe::engine {

class SceneContext {
 public:
  template <typename T>
  bool has_context() const {
    return m_contexts.find(typeid(T).hash_code()) != m_contexts.end();
  }

  template <typename T, typename... Args>
  T* emplace(Args&&... args) {
    auto hash = typeid(T).hash_code();
    m_contexts[hash] = Any::create<T>(std::forward<Args>(args)...);
    return m_contexts[hash].get<T>();
  }

  template <typename T>
  T* get() {
    auto it = m_contexts.find(typeid(T).hash_code());
    return (it != m_contexts.end()) ? it->second.template get<T>() : nullptr;
  }

  template <typename T>
  const T* get() const {
    auto it = m_contexts.find(typeid(T).hash_code());
    return (it != m_contexts.end()) ? it->second.template get<T>() : nullptr;
  }

  void add_refresh(std::function<void()> refresh) { m_refreshs.push_back(std::move(refresh)); }

  void refresh() {
    for (auto& refresh : m_refreshs) {
      refresh();
    }
  }

 private:
  stl::unordered_map<size_t, Any> m_contexts;
  stl::vector<std::function<void()>> m_refreshs;
};

}  // namespace fe::engine