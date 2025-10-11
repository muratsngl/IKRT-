#pragma once
#include <glm/glm.hpp>

// Point light with position-based illumination and attenuation
struct PointLight {
    glm::vec3 position;
    glm::vec3 color;        // RGB color
    float intensity;        // Light intensity multiplier
};

// Directional light (like sunlight) with parallel rays
struct DirectionalLight {
    glm::vec3 direction;    // Light direction (normalized)
    glm::vec3 color;        // RGB color
    float intensity;        // Light intensity multiplier
};

// Spot light with cone-shaped illumination
struct SpotLight {
    glm::vec3 position;     // Light position
    glm::vec3 direction;    // Light direction (normalized)
    glm::vec3 color;        // RGB color
    float intensity;        // Light intensity multiplier
    float innerConeAngle;   // Inner cone angle (full intensity)
    float outerConeAngle;   // Outer cone angle (zero intensity)
};