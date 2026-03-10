#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "core/macros.h"

namespace fe::engine {

struct Transform {
  glm::vec3 position{0.0f, 0.0f, 0.0f};
  glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
  glm::vec3 scale{1.0f, 1.0f, 1.0f};

  FE_FINLINE glm::mat4 getMatrix() const {
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
};
}  // namespace fe::engine