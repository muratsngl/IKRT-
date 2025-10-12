#pragma once
#include <glm/glm.hpp>

// Point light with position-based illumination and attenuation
struct PointLight {
    glm::vec3 position;     // offset 0, size 12
    float intensity;        // offset 12, size 4
    glm::vec3 color;        // offset 16, size 12
    float _padding;         // offset 28, size 4 (makes struct 32 bytes)
};

// Directional light (like sunlight) with parallel rays
struct DirectionalLight {
    glm::vec3 direction;    // Light direction (normalized)
    glm::vec3 color;        // RGB color
    float intensity;        // Light intensity multiplier
};

// Spot light with cone-shaped illumination
struct SpotLight {
    glm::vec3 position;     // offset 0, size 12
    float intensity;        // offset 12, size 4
    glm::vec3 direction;    // offset 16, size 12
    float innerCone;        // offset 28, size 4
    glm::vec3 color;        // offset 32, size 12
    float outerCone;        // offset 44, size 4
    // Struct is already 48 bytes, a multiple of 16. No extra padding needed.
};