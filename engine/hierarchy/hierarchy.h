#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "common.h"

namespace fe::engine::ecs {
struct Hierarchy {
  entt::entity parent{entt::null};
  entt::entity first_child{entt::null};
  entt::entity prev_sibling{entt::null};
  entt::entity next_sibling{entt::null};
  size_t children_count{0};
};

class HierarchyTool {
 public:
  static void set_parent(entt::registry& reg, entt::entity child, entt::entity parent) {
    auto& child_h = reg.get_or_emplace<Hierarchy>(child);

    if (child_h.parent == parent)
      return;

    if (child_h.parent != entt::null) {
      remove_from_parent(reg, child);
    }

    if (parent == entt::null)
      return;

    auto& parent_h = reg.get_or_emplace<Hierarchy>(parent);
    child_h.parent = parent;

    child_h.next_sibling = parent_h.first_child;
    child_h.prev_sibling = entt::null;

    if (parent_h.first_child != entt::null) {
      reg.get<Hierarchy>(parent_h.first_child).prev_sibling = child;
    }

    parent_h.first_child = child;
    parent_h.children_count++;
  }

  static void remove_from_parent(entt::registry& reg, entt::entity child) {
    auto* child_h = reg.try_get<Hierarchy>(child);
    if (!child_h || child_h->parent == entt::null)
      return;

    auto& parent_h = reg.get<Hierarchy>(child_h->parent);

    if (child_h->prev_sibling != entt::null) {
      reg.get<Hierarchy>(child_h->prev_sibling).next_sibling = child_h->next_sibling;
    } else {
      parent_h.first_child = child_h->next_sibling;
    }

    if (child_h->next_sibling != entt::null) {
      reg.get<Hierarchy>(child_h->next_sibling).prev_sibling = child_h->prev_sibling;
    }

    child_h->parent = entt::null;
    child_h->prev_sibling = entt::null;
    child_h->next_sibling = entt::null;
    parent_h.children_count--;
  }

  template <typename Func>
  static void each_child(const entt::registry& reg, entt::entity entity, Func&& func) {
    auto* h = reg.try_get<Hierarchy>(entity);
    if (!h)
      return;

    entt::entity curr = h->first_child;
    while (curr != entt::null) {
      entt::entity next = reg.get<Hierarchy>(curr).next_sibling;
      func(curr);
      curr = next;
    }
  }

  static void on_destroy(entt::registry& reg, entt::entity entity) {
    remove_from_parent(reg, entity);

    each_child(reg, entity, [&](entt::entity child) {
      auto& child_h = reg.get<Hierarchy>(child);
      child_h.parent = entt::null;
      child_h.prev_sibling = entt::null;
      child_h.next_sibling = entt::null;
    });
  }
};

}  // namespace fe::engine::ecs