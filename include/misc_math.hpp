#ifndef MISC_MATH_HPP
#define MISC_MATH_HPP

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// Interpolates between two transformation matrices using SLERP for rotation
// and LERP for translation/scale.
// alpha: 0.0 (start) to 1.0 (end)
glm::mat4 interpolate_transform(const glm::mat4& start, const glm::mat4& end, float alpha);

// Interpolates between two position vectors (LERP)
glm::vec3 interpolate_position(const glm::vec3& start, const glm::vec3& end, float alpha);

#endif
