#pragma once

#include "component.h"

#include "engine/ecs/paramTypes.h"

namespace fe::engine::hierarchy {

class HierarchyOps {
  using ComWriter = ecs::ComponentWriter<Hierarchy, Transform>;
  ComWriter& m_writer;

 public:
  HierarchyOps(ComWriter& writer) : m_writer(writer) {}

  void set_parent(entt::entity child, entt::entity parent) {
    if (child == parent)
      return;

    auto& child_h = m_writer.get_or_emplace<Hierarchy>(child);

    if (child_h.parent == parent)
      return;

    if (child_h.parent != entt::null) {
      remove_from_parent(child);
    }

    if (parent == entt::null)
      return;

    auto& parent_h = m_writer.get_or_emplace<Hierarchy>(parent);
    child_h.parent = parent;

    child_h.next_sibling = parent_h.first_child;
    child_h.prev_sibling = entt::null;

    if (parent_h.first_child != entt::null) {
      m_writer.get<Hierarchy>(parent_h.first_child).prev_sibling = child;
    }

    parent_h.first_child = child;
    parent_h.children_count++;
  }

  void remove_from_parent(entt::entity child) {
    auto* child_h = m_writer.try_get<Hierarchy>(child);
    if (!child_h || child_h->parent == entt::null)
      return;

    auto& parent_h = m_writer.get<Hierarchy>(child_h->parent);

    if (child_h->prev_sibling != entt::null) {
      m_writer.get<Hierarchy>(child_h->prev_sibling).next_sibling = child_h->next_sibling;
    } else {
      parent_h.first_child = child_h->next_sibling;
    }

    if (child_h->next_sibling != entt::null) {
      m_writer.get<Hierarchy>(child_h->next_sibling).prev_sibling = child_h->prev_sibling;
    }

    child_h->parent = entt::null;
    child_h->prev_sibling = entt::null;
    child_h->next_sibling = entt::null;
    parent_h.children_count--;
  }

  template <typename Func>
  void each_child(entt::entity entity, Func&& func) {
    auto* h = m_writer.try_get<Hierarchy>(entity);
    if (!h)
      return;

    entt::entity curr = h->first_child;
    while (curr != entt::null) {
      entt::entity next = m_writer.get<Hierarchy>(curr).next_sibling;
      func(curr);
      curr = next;
    }
  }

  void on_destroy(entt::entity entity) {
    remove_from_parent(entity);

    each_child(entity, [&](entt::entity child) {
      auto& child_h = m_writer.get<Hierarchy>(child);
      child_h.parent = entt::null;
      child_h.prev_sibling = entt::null;
      child_h.next_sibling = entt::null;
    });
  }
};

}  // namespace fe::engine::hierarchy