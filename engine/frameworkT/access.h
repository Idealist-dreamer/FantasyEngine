#pragma once

#include <typeindex>

#include "foundation/container/stl.h"

namespace fe::engine {

struct RootAccess {};

class Access {
 public:
  enum class Mode : uint8_t { None = 0, Read = 1, Write = 2 };

  explicit Access(std::type_index t = typeid(RootAccess)) : m_type(t) {}

  Access& read() {
    if (m_mode == Mode::None) m_mode = Mode::Read;
    return *this;
  }

  Access& write() {
    m_mode = Mode::Write;
    return *this;
  }

  template <typename T>
  Access& child() {
    std::type_index id(typeid(T));
    auto& child_ptr = m_children[id];
    if (!child_ptr) {
      child_ptr = stl::make_unique<Access>(id);
    }
    return *child_ptr;
  }

  void merge(const Access& other) {
    if (other.m_mode == Mode::Write)
      m_mode = Mode::Write;
    else if (other.m_mode == Mode::Read && m_mode == Mode::None)
      m_mode = Mode::Read;

    for (const auto& [id, other_child] : other.m_children) {
      auto& my_child = m_children[id];
      if (!my_child) my_child = stl::make_unique<Access>(id);
      my_child->merge(*other_child);
    }
  }

  void bake() {
    m_tree_max_mode = m_mode;
    for (const auto& [_, child] : m_children) {
      child->bake();
      if (child->m_tree_max_mode == Mode::Write)
        m_tree_max_mode = Mode::Write;
      else if (child->m_tree_max_mode == Mode::Read &&
               m_tree_max_mode == Mode::None)
        m_tree_max_mode = Mode::Read;
    }
  }

  bool is_conflict(const Access& other) const {
    if (m_type != other.m_type) return false;

    if (!check_conflict(m_tree_max_mode, other.m_tree_max_mode)) return false;

    if (check_conflict(m_mode, other.m_tree_max_mode)) return true;
    if (check_conflict(other.m_mode, m_tree_max_mode)) return true;

    for (const auto& [id, my_child] : m_children) {
      auto it = other.m_children.find(id);
      if (it != other.m_children.end()) {
        if (my_child->is_conflict(*(it->second))) {
          return true;
        }
      }
    }
    return false;
  }

 private:
  static bool check_conflict(Mode a, Mode b) {
    if (a == Mode::None || b == Mode::None) return false;
    return a == Mode::Write || b == Mode::Write;
  }

  std::type_index m_type;
  Mode m_mode = Mode::None;
  Mode m_tree_max_mode = Mode::None;
  stl::unordered_map<std::type_index, stl::unique_ptr<Access>> m_children;
};

}  // namespace fe::engine
