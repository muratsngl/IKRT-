#pragma once

#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// Matrix decomposition and composition utilities
namespace MatrixUtils {
    glm::vec3 extractPosition(const glm::mat4& matrix);
    glm::quat extractRotation(const glm::mat4& matrix);
    glm::vec3 extractScale(const glm::mat4& matrix);
    glm::mat4 composeMatrix(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale);
}

// Scene serialization functions
namespace SceneSerializer {
    bool saveScene(const std::string& filepath);
    bool loadScene(const std::string& filepath);
    void clearScene();
}
