#pragma once

#include <functional>
#include <typeindex>

#include "foundation/container/stl.h"
#include "foundation/utility/any.h"

namespace fe::engine {

class Blackboard {
 public:
  Blackboard() = default;
  ~Blackboard() = default;

  Blackboard(const Blackboard&) = delete;
  Blackboard& operator=(const Blackboard&) = delete;

  template <typename T>
  bool has() const {
    return m_data.find(typeid(T)) != m_data.end();
  }

  template <typename T, typename... Args>
  T& emplace(Args&&... args) {
    auto type = std::type_index(typeid(T));
    m_data[type] = Any::create<T>(std::forward<Args>(args)...);
    return *m_data[type].template get<T>();
  }

  template <typename T>
  T& get() {
    return *m_data.at(std::type_index(typeid(T))).template get<T>();
  }

  template <typename T>
  const T& get() const {
    return *m_data.at(std::type_index(typeid(T))).template get<T>();
  }

  void add_refresh(std::function<void()> refresh) {
    m_refreshs.push_back(std::move(refresh));
  }

  void refresh() {
    for (auto& refresh : m_refreshs) {
      refresh();
    }
  }

 private:
  stl::unordered_map<std::type_index, Any> m_data;
  stl::vector<std::function<void()>> m_refreshs;
};

}  // namespace fe::engine
