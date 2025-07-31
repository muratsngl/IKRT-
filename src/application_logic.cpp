#include "include/application_logic.hpp"
#include "include/shared_memory.hpp"
#include "include/model_loader.hpp"
#include "include/FABRIK.h"
#include <GLFW/glfw3.h>

static ApplicationState app_state;

void init_application_state() {
    // Initialize target positions
    app_state.targetPositionIndex = glm::vec3(-1.2f, 2.0f, 0.0f);
    app_state.targetPositionMiddle = glm::vec3(-0.3f, -1.2f, 0.0f);
    app_state.targetPositionRing = glm::vec3(1.6f, 2.0f, 0.0f);
    app_state.targetPositionPinky = glm::vec3(0.5f, -1.2f, 0.0f);
    
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
    app_state.deltaRoot += !finger_data.rootLock? glm::vec3(8.0f * finger_data.deltaXRoot,
                                   5.32f * finger_data.deltaYRoot,
                                   finger_data.deltaZRoot): glm::vec3(0.0f); // Added for root movement
    
    // Update target positions
    app_state.targetPositionIndex += app_state.deltaIndex;
    app_state.targetPositionRing += app_state.deltaRing;
    app_state.targetPositionMiddle += app_state.deltaMiddle;
    app_state.targetPositionPinky += app_state.deltaPinky;
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
