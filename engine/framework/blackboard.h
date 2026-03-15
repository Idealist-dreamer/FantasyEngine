#pragma once

#include "foundation/utility/any.h"
#include "foundation/utility/meta.h"
#include "foundation/utility/type.h"
#include "foundation/container/stl.h"

namespace fe::engine {

class Blackboard {
 public:
  Blackboard() = default;
  ~Blackboard() = default;

  Blackboard(const Blackboard&) = delete;
  Blackboard& operator=(const Blackboard&) = delete;

  template <typename T>
  bool has() const {
    return m_data.find(get_type_id<T>()) != m_data.end();
  }

  template <typename T, typename... Args>
  T& emplace_or_replace(Args&&... args) {
    auto [it, inserted] = m_data.insert_or_assign(
        get_type_id<T>(), Any::create<T>(std::forward<Args>(args)...));
    return *it->second.template get<T>();
  }

  template <typename T>
  T* try_get() {
    if (auto it = m_data.find(get_type_id<T>()); it != m_data.end()) {
      return it->second.template get<T>();
    }
    return nullptr;
  }

  template <typename T>
  const T* try_get() const {
    if (auto it = m_data.find(get_type_id<T>()); it != m_data.end()) {
      return it->second.template get<T>();
    }
    return nullptr;
  }

  template <typename T>
  T& get() {
    return *m_data.at(get_type_id<T>()).template get<T>();
  }

  template <typename T>
  const T& get() const {
    return *m_data.at(get_type_id<T>()).template get<T>();
  }

  void clear() { m_data.clear(); }

 private:
  stl::unordered_map<TypeId, Any> m_data;
};

}  // namespace fe::engine