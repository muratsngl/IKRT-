#ifndef COLLISION_HPP
#define COLLISION_HPP

#include <glm/matrix.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>


typedef enum shapeType{
    SPHERE,
    OBB,
    AABB
} shapeType;

typedef struct Sphere{float radius; glm::vec3 center;} Sphere;
typedef struct Obb{glm::vec3 center; glm::vec3 halfExtents; glm::quat rotation;} Obb;
typedef struct Aabb{glm::vec3 min; glm::vec3 max;} Aabb;

// Transform a local-space AABB to world-space using a model matrix
Aabb transformAABB(const Aabb& localAABB, const glm::mat4& modelMatrix);
typedef struct Shape{
  shapeType type;
  union{
      Sphere sphere;
      Obb obb;
      Aabb aabb;
  };
} Shape;



bool check_collision(const Shape& interactor_box,const std::vector<Shape>& scene_element_boxes);

#endif