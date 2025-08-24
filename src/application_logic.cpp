#include "include/application_logic.hpp"
#include "include/shared_memory.hpp"
#include "include/model_loader.hpp"
#include "include/FABRIK.h"
#include "collision.hpp"
#include <GLFW/glfw3.h>
#include <cmath>
#include <iostream>


static ApplicationState app_state;

void init_application_state() {
    // Initialize target positions
    app_state.targetPositionIndex = glm::vec3(-1.2f, 2.0f, 0.0f);
    app_state.targetPositionMiddle = glm::vec3(-0.3f, -1.2f, 0.0f);
    app_state.targetPositionRing = glm::vec3(1.6f, 2.0f, 0.0f);
    app_state.targetPositionPinky = glm::vec3(0.5f, -1.2f, 0.0f);
    app_state.targetPositionRoot = glm::vec3(0.0f, 0.0f, 0.0f); // Added for root movement
    
    // Initialize deltas
    app_state.deltaIndex = glm::vec3(0.0f);
    app_state.deltaMiddle = glm::vec3(0.0f);
    app_state.deltaRing = glm::vec3(0.0f);
    app_state.deltaPinky = glm::vec3(0.0f);
    app_state.deltaRoot = glm::vec3(0.0f); // Added for root movement
    
    // Initialize timing
    app_state.deltaTime = 0.0f;
    app_state.lastFrame = 0.0f;
}

void update_finger_positions() {
    float currentFrame = static_cast<float>(glfwGetTime());
    app_state.deltaTime = currentFrame - app_state.lastFrame;
    app_state.lastFrame = currentFrame;
}

void calculate_deltas() {
    const FingerData& finger_data = get_finger_data();
    
    // Calculate movement deltas with scaling factors
    app_state.deltaIndex = glm::vec3(4.0f * finger_data.deltaXIndex, 
                                   2.66f * finger_data.deltaYIndex, 
                                   finger_data.deltaZIndex);
    
    app_state.deltaRing = glm::vec3(4.0f * finger_data.deltaXRing, 
                                  2.66f * finger_data.deltaYRing, 
                                  finger_data.deltaZRing);
    
    app_state.deltaMiddle = glm::vec3(4.0f * finger_data.deltaXMiddle, 
                                    2.66f * finger_data.deltaYMiddle, 
                                    finger_data.deltaZMiddle);
    
    app_state.deltaPinky = glm::vec3(4.0f * finger_data.deltaXPinky, 
                                   2.66f * finger_data.deltaYPinky, 
                                   finger_data.deltaZPinky);
    app_state.deltaRoot = !finger_data.rootLock? glm::vec3(8.0f * finger_data.deltaXRoot,
                                   5.32f * finger_data.deltaYRoot,
                                   finger_data.deltaZRoot): glm::vec3(0.0f); // Added for root movement
    
    // Update target positions - apply deltaRoot directly to all targets
    app_state.targetPositionIndex += app_state.deltaIndex - app_state.deltaRoot*0.5f;
    app_state.targetPositionRing += app_state.deltaRing - app_state.deltaRoot*0.5f;
    app_state.targetPositionMiddle += app_state.deltaMiddle - app_state.deltaRoot*0.5f;
    app_state.targetPositionPinky += app_state.deltaPinky - app_state.deltaRoot*0.5f;
    app_state.targetPositionRoot += app_state.deltaRoot; // Added for root movement
}

void apply_fabrik() {
    const InteractorModelData& model_data = get_interactor_model_data();
    
    // Apply FABRIK to each limb
    simple_fabrik_routine_indexed(const_cast<std::vector<glm::vec3>&>(model_data.bind_pose_positions),
                                 app_state.targetPositionIndex, 
                                 model_data.right_arm_indices);
    
    simple_fabrik_routine_indexed(const_cast<std::vector<glm::vec3>&>(model_data.bind_pose_positions),
                                 app_state.targetPositionMiddle, 
                                 model_data.right_leg_indices);
    
    simple_fabrik_routine_indexed(const_cast<std::vector<glm::vec3>&>(model_data.bind_pose_positions),
                                 app_state.targetPositionRing, 
                                 model_data.left_arm_indices);
    
    simple_fabrik_routine_indexed(const_cast<std::vector<glm::vec3>&>(model_data.bind_pose_positions),
                                 app_state.targetPositionPinky, 
                                 model_data.left_leg_indices);
}

void rearrange_finger_positions_based_on_collision(){
    const InteractorModelData& model_data = get_interactor_model_data();
    extern std::vector<Shape> scene_element_boxes;
    
    // Create OBB for bone segment between bone i and i+1
    auto createBoneOBB = [&](int boneIndex1, int boneIndex2) -> Shape {
        Shape boneShape;
        boneShape.type = OBB;
        
        glm::vec3 pos1 = model_data.bind_pose_positions[boneIndex1];
        glm::vec3 pos2 = model_data.bind_pose_positions[boneIndex2];
        
        // Center is midpoint between bones
        glm::vec3 center = (pos1 + pos2) * 0.5f;
        
        // Direction vector between bones
        glm::vec3 direction = pos2 - pos1;
        float length = glm::length(direction);
        
        if (length > 0.0001f) {
            direction = glm::normalize(direction);
        } else {
            direction = glm::vec3(1, 0, 0); // Default direction
        }
        
        // Create rotation to align with direction vector
        glm::vec3 up = direction;  // Bone direction becomes the up vector
        glm::vec3 forward = glm::vec3(0, 0, 1);  // Default forward
        
        // If bone direction is too close to forward, use a different forward
        if (abs(glm::dot(up, forward)) > 0.99f) {
            forward = glm::vec3(1, 0, 0);
        }
        
        glm::vec3 right = glm::normalize(glm::cross(up, forward));
        forward = glm::normalize(glm::cross(right, up));
        
        // Build rotation matrix: right=X, up=Y, forward=Z
        glm::mat3 rotMatrix(right, up, forward);
        glm::quat rotation = glm::quat_cast(rotMatrix);
        
        // Set OBB properties
        boneShape.obb.center = center;
        boneShape.obb.rotation = rotation;
        // Half extents: small width/depth (X,Z), length along bone direction (Y)
        boneShape.obb.halfExtents = glm::vec3(0.05f, length * 0.5f, 0.05f);

        return boneShape;
    };
    
    // Check collision for specific finger chain
    auto checkFingerCollision = [&](const std::vector<unsigned short>& indices, glm::vec3& targetPosition, const glm::vec3& delta) {
        for (size_t i = 0; i < indices.size() - 1; i++) {
            Shape boneOBB = createBoneOBB(indices[i], indices[i + 1]);
            
            if (check_collision(boneOBB, scene_element_boxes)) {
                // Collision detected, subtract -2 * delta from target position
                std::cout << "Collision detected on finger segment!" << std::endl;
                targetPosition -= 2.0f * delta;
                break; // Only apply once per finger
            }
        }
    };
    
    // Check collisions for each finger using app_state finger names
    checkFingerCollision(model_data.right_arm_indices, app_state.targetPositionIndex, app_state.deltaIndex);
    checkFingerCollision(model_data.right_leg_indices, app_state.targetPositionMiddle, app_state.deltaMiddle);
    checkFingerCollision(model_data.left_arm_indices, app_state.targetPositionRing, app_state.deltaRing);
    checkFingerCollision(model_data.left_leg_indices, app_state.targetPositionPinky, app_state.deltaPinky);
};



void update_transforms() {
    const InteractorModelData& model_data = get_interactor_model_data();
    
    // Update bone transforms for each limb
    update_bone_transforms(model_data.right_arm_indices);
    update_bone_transforms(model_data.left_arm_indices);
    update_bone_transforms(model_data.right_leg_indices);
    update_bone_transforms(model_data.left_leg_indices);
}

ApplicationState& get_application_state() {
    return app_state;
}
