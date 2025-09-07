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

// OBB-OBB intersection test
bool intersectOBB(const Obb& a, const Obb& b);

// Helper functions for OBB intersection
bool testSeparatingAxis(const glm::vec3& axis, const Obb& a, const Obb& b, const glm::vec3& T);

// Helper function to create OBB from mesh bounds and transform
Obb createOBBFromBounds(const glm::vec3& minBounds, const glm::vec3& maxBounds, const glm::mat4& transform);
typedef struct Shape{
  shapeType type;
  int id; // ID to identify which model this shape belongs to
  union{
      Sphere sphere;
      Obb obb;
      Aabb aabb;
  };
} Shape;



bool check_collision(const Shape& interactor_box,const std::vector<Shape>& scene_element_boxes);
int check_collision_with_id(const Shape& interactor_box,const std::vector<Shape>& scene_element_boxes);

#endif