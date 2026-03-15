#pragma once

#include <tuple>
#include <functional>

#include "blackboard.h"

namespace fe::engine {

class AccessInfo {
  enum class Mode : uint8_t { None = 0, Read = 1, Write = 2 };

 public:
  explicit AccessInfo(TypeId t = get_type_id<Blackboard>()) : m_type(t) {}

  AccessInfo& read() {
    if (m_mode == Mode::None) m_mode = Mode::Read;
    return *this;
  }

  AccessInfo& write() {
    m_mode = Mode::Write;
    return *this;
  }

  template <typename T>
  AccessInfo& child() {
    TypeId id = get_type_id<T>();
    auto& child_ptr = m_children[id];
    if (!child_ptr) child_ptr = stl::make_unique<AccessInfo>(id);
    return *child_ptr;
  }

  void merge(const AccessInfo& other) {
    if (other.m_mode == Mode::Write)
      m_mode = Mode::Write;
    else if (other.m_mode == Mode::Read && m_mode == Mode::None)
      m_mode = Mode::Read;

    for (const auto& [id, other_child] : other.m_children) {
      auto& my_child = m_children[id];
      if (!my_child) my_child = stl::make_unique<AccessInfo>(id);
      my_child->merge(*other_child);
    }
  }

  void bake() {
    m_intent_mode = m_mode;
    for (const auto& [_, child] : m_children) {
      child->bake();
      if (child->m_intent_mode == Mode::Write)
        m_intent_mode = Mode::Write;
      else if (child->m_intent_mode == Mode::Read &&
               m_intent_mode == Mode::None)
        m_intent_mode = Mode::Read;
    }
  }

  bool is_conflict(const AccessInfo& other) const {
    if (m_type != other.m_type) return false;

    if (!is_mode_conflict(m_intent_mode, other.m_intent_mode)) return false;

    if (is_mode_conflict(m_mode, other.m_mode)) return true;

    if (m_mode == Mode::Write && other.m_intent_mode != Mode::None) return true;
    if (other.m_mode == Mode::Write && m_intent_mode != Mode::None) return true;

    for (const auto& [id, my_child] : m_children) {
      auto it = other.m_children.find(id);
      if (it != other.m_children.end()) {
        if (my_child->is_conflict(*(it->second))) return true;
      }
    }

    return false;
  }

 private:
  static bool is_mode_conflict(Mode a, Mode b) {
    if (a == Mode::None || b == Mode::None) return false;
    return a == Mode::Write || b == Mode::Write;
  }

  TypeId m_type;
  Mode m_mode = Mode::None;
  Mode m_intent_mode = Mode::None;
  stl::unordered_map<TypeId, stl::unique_ptr<AccessInfo>> m_children;
};

template <typename T>
struct AccessTraits {
  static void declare(AccessInfo& acc) { AccessTraits<const T&>::declare(acc); }
  static void prepare(Blackboard& bb, uint32_t p) {
    AccessTraits<const T&>::prepare(bb, p);
  }
  static const T& fetch(Blackboard& bb, uint32_t p) {
    return AccessTraits<const T&>::fetch(bb, p);
  }
};

template <typename T>
struct AccessTraits<T&> {
  using CleanT = meta::clean_t<T>;
  static void declare(AccessInfo& acc) { acc.child<CleanT>().write(); }
  static void prepare(Blackboard& bb, uint32_t) {
    if (!bb.has<CleanT>()) bb.emplace_or_replace<CleanT>();
  }
  static CleanT& fetch(Blackboard& bb, uint32_t) { return bb.get<CleanT>(); }
};

template <typename T>
struct AccessTraits<const T&> {
  using CleanT = meta::clean_t<T>;
  static void declare(AccessInfo& acc) { acc.child<CleanT>().read(); }
  static void prepare(Blackboard& bb, uint32_t) {
    if (!bb.has<CleanT>()) bb.emplace_or_replace<CleanT>();
  }
  static const CleanT& fetch(Blackboard& bb, uint32_t) {
    return bb.get<CleanT>();
  }
};

template <>
struct AccessTraits<Blackboard&> {
  static void declare(AccessInfo& acc) { acc.write(); }
  static void prepare(Blackboard&, uint32_t) {}
  static Blackboard& fetch(Blackboard& bb, uint32_t) { return bb; }
};

template <>
struct AccessTraits<const Blackboard&> {
  static void declare(AccessInfo& acc) { acc.read(); }
  static void prepare(Blackboard&, uint32_t) {}
  static const Blackboard& fetch(Blackboard& bb, uint32_t) { return bb; }
};

// ============================================================================
// CleanupRegistry: 用于注册在 Cleanup 阶段执行的回调
// ============================================================================
struct CleanupRegistry {
  stl::vector<std::function<void(Blackboard&)>> cleanups;

  template <typename Func>
  void register_cleanup(Func&& func) {
    cleanups.push_back(std::forward<Func>(func));
  }

  void execute(Blackboard& bb) {
    for (auto& cleanup : cleanups) {
      cleanup(bb);
    }
  }
};

template <>
struct AccessTraits<CleanupRegistry&> {
  static void declare(AccessInfo& acc) { acc.child<CleanupRegistry>().write(); }
  static void prepare(Blackboard& bb, uint32_t) {
    if (!bb.has<CleanupRegistry>()) bb.emplace_or_replace<CleanupRegistry>();
  }
  static CleanupRegistry& fetch(Blackboard& bb, uint32_t) {
    return bb.get<CleanupRegistry>();
  }
};

struct AccessOps {
  template <typename... Args>
  static AccessInfo declare_all() {
    AccessInfo root_acc;
    (AccessTraits<Args>::declare(root_acc), ...);
    root_acc.bake();
    return root_acc;
  }

  template <typename... Args>
  static void prepare_all(Blackboard& bb, uint32_t pass_id) {
    (AccessTraits<Args>::prepare(bb, pass_id), ...);
  }

  template <typename... Args>
  static auto fetch_all(Blackboard& bb, uint32_t pass_id) {
    return std::tuple<decltype(AccessTraits<Args>::fetch(bb, pass_id))...>(
        AccessTraits<Args>::fetch(bb, pass_id)...);
  }
};

}  // namespace fe::engine