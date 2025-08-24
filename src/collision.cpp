#include "collision.hpp"
#include <cmath>
#include <iostream>

glm::mat4 model(1.0f);
glm::quat rotationX = glm::angleAxis(glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
glm::quat rotationZ = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, -1.0f));

// Transform a local-space AABB to world-space using a model matrix
Aabb transformAABB(const Aabb& localAABB, const glm::mat4& modelMatrix) {
    glm::vec3 corners[8] = {
        {localAABB.min.x, localAABB.min.y, localAABB.min.z},
        {localAABB.max.x, localAABB.min.y, localAABB.min.z},
        {localAABB.min.x, localAABB.max.y, localAABB.min.z},
        {localAABB.max.x, localAABB.max.y, localAABB.min.z},
        {localAABB.min.x, localAABB.min.y, localAABB.max.z},
        {localAABB.max.x, localAABB.min.y, localAABB.max.z},
        {localAABB.min.x, localAABB.max.y, localAABB.max.z},
        {localAABB.max.x, localAABB.max.y, localAABB.max.z}
    };

    glm::vec3 newMin = glm::vec3(modelMatrix * glm::vec4(corners[0], 1.0f));
    glm::vec3 newMax = newMin;
    for (int i = 1; i < 8; ++i) {
        glm::vec3 transformed = glm::vec3(modelMatrix * glm::vec4(corners[i], 1.0f));
        newMin = glm::min(newMin, transformed);
        newMax = glm::max(newMax, transformed);
    }
    return {newMin, newMax};
}

// AABB-OBB intersection test using separating axis theorem
bool testAABBOBB(const Aabb& aabb, const Obb& obb) {
    // Get OBB axes (from rotation quaternion)
    glm::mat3 rotation = glm::mat3_cast(obb.rotation);
    glm::vec3 obbAxes[3] = {
        rotation[0], // X axis
        rotation[1], // Y axis
        rotation[2]  // Z axis
    };
    
    // AABB axes (world aligned)
    glm::vec3 aabbAxes[3] = {
        glm::vec3(1, 0, 0),
        glm::vec3(0, 1, 0),
        glm::vec3(0, 0, 1)
    };
    
    // Get AABB center and half extents
    glm::vec3 aabbCenter = (aabb.min + aabb.max) * 0.5f;
    glm::vec3 aabbHalfExtents = (aabb.max - aabb.min) * 0.5f;
    
    // Distance between centers
    glm::vec3 distance = obb.center - aabbCenter;
    
    // Test all 15 potential separating axes
    glm::vec3 axes[15];
    
    // 3 AABB axes
    for (int i = 0; i < 3; i++) {
        axes[i] = aabbAxes[i];
    }
    
    // 3 OBB axes
    for (int i = 0; i < 3; i++) {
        axes[3 + i] = obbAxes[i];
    }
    
    // 9 cross products of AABB and OBB axes
    int axisIndex = 6;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            axes[axisIndex++] = glm::cross(aabbAxes[i], obbAxes[j]);
        }
    }
    
    // Test each axis
    for (int i = 0; i < 15; i++) {
        glm::vec3 axis = axes[i];
        
        // Skip degenerate axes (from parallel edges)
        if (glm::length(axis) < 0.0001f) continue;
        axis = glm::normalize(axis);
        
        // Project AABB onto axis
        float aabbProjection = 
            std::abs(glm::dot(aabbHalfExtents.x * aabbAxes[0], axis)) +
            std::abs(glm::dot(aabbHalfExtents.y * aabbAxes[1], axis)) +
            std::abs(glm::dot(aabbHalfExtents.z * aabbAxes[2], axis));
        
        // Project OBB onto axis
        float obbProjection = 
            std::abs(glm::dot(obb.halfExtents.x * obbAxes[0], axis)) +
            std::abs(glm::dot(obb.halfExtents.y * obbAxes[1], axis)) +
            std::abs(glm::dot(obb.halfExtents.z * obbAxes[2], axis));
        
        // Project distance onto axis
        float distanceProjection = std::abs(glm::dot(distance, axis));
        
        // If projected distance is greater than sum of projections, no overlap
        if (distanceProjection > aabbProjection + obbProjection) {
            return false; // Separating axis found
        }
    }
    
    return true; // No separating axis found, intersection exists
}

bool check_collision(const Shape& interactor_box, const std::vector<Shape>& scene_element_boxes)
{
    // Implement collision detection logic here
    for (const auto& scene_element_box : scene_element_boxes) {
        
        //model = glm::mat4(rotationX * rotationZ) * model;
        model = glm::translate(model, glm::vec3(0.f));
        // AABB vs AABB
        if(interactor_box.type == AABB && scene_element_box.type == AABB) {
            const Aabb& interactor_aabb = interactor_box.aabb;
            const Aabb scene_element_aabb = transformAABB(scene_element_box.aabb, model);

            // Check for overlap in all three axes
            if (interactor_aabb.max.x >= scene_element_aabb.min.x &&
                interactor_aabb.min.x <= scene_element_aabb.max.x &&
                interactor_aabb.max.y >= scene_element_aabb.min.y &&
                interactor_aabb.min.y <= scene_element_aabb.max.y &&
                interactor_aabb.max.z >= scene_element_aabb.min.z &&
                interactor_aabb.min.z <= scene_element_aabb.max.z) {
                return true; // Collision detected
                
            }
        }
        // AABB vs OBB
        else if(interactor_box.type == AABB && scene_element_box.type == OBB) {
            const Aabb& interactor_aabb = interactor_box.aabb;
            const Obb& scene_element_obb = scene_element_box.obb;
            
            if (testAABBOBB(interactor_aabb, scene_element_obb)) {
                std::cout << "AABB vs OBB collision detected" << std::endl;
                return true; // Collision detected
            }
        }
        // OBB vs AABB  
        else if(interactor_box.type == OBB && scene_element_box.type == AABB) {
            const Obb& interactor_obb = interactor_box.obb;
            const Aabb scene_element_aabb = transformAABB(scene_element_box.aabb, model);
            
            if (testAABBOBB(scene_element_aabb, interactor_obb)) {
                std::cout << "AABB vs OBB collision detected" << std::endl;
                return true; // Collision detected
            }
        }
        // OBB vs OBB (placeholder - can implement if needed)
        else if(interactor_box.type == OBB && scene_element_box.type == OBB) {
            // TODO: Implement OBB-OBB intersection if needed
        }
    }
    
    return false;
}