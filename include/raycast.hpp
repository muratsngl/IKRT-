#ifndef RAYCAST_HPP
#define RAYCAST_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "collision.hpp"

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

// Generate a ray from screen coordinates (mouse position) into world space
Ray generate_ray(float mouse_x, float mouse_y, 
                 const glm::mat4& view, 
                 const glm::mat4& projection, 
                 const glm::vec3& camera_pos,
                 int screen_width, 
                 int screen_height);

// Intersect ray with a collection of shapes and return the closest hit model ID
// Returns -1 if no intersection found
int intersect_ray(const Ray& ray, const std::vector<Shape>& shapes);

// Helper function for ray-AABB intersection test
bool ray_aabb_intersect(const Ray& ray, const Aabb& aabb, float& t_min, float& t_max);

#endif