#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "common.h"

namespace fe::engine::ecs {

struct Transform {
  glm::vec3 position{0.0f, 0.0f, 0.0f};
  glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
  glm::vec3 scale{1.0f, 1.0f, 1.0f};

  glm::mat4 getMatrix() const {
    glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 R = glm::mat4_cast(glm::normalize(rotation));
    glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
    return T * R * S;
  }
};

struct ModelMatrix {
  glm::mat4 value;
};

struct Hierarchy {
  entt::entity parent{entt::null};
  entt::entity first_child{entt::null};
  entt::entity prev_sibling{entt::null};
  entt::entity next_sibling{entt::null};
  size_t children_count{0};
};

}  // namespace fe::engine::ecs