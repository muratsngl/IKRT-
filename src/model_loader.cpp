#include "include/model_loader.hpp"
#include "include/model_bones.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>

static InteractorModelData model_data;
static InteractorModel* current_model = nullptr;

bool load_interactor_model(const char* path) {
    try {
        current_model = new InteractorModel("assets/models/MAN_WITH_CORRECT_BONES_AND_SYSTEM_REALLY.dae");
        
        // Copy data from the model
        model_data.bind_pose_positions = current_model->bindPosePositions;
        model_data.bind_pose_positions_original = model_data.bind_pose_positions;
        model_data.bind_pose_matrices = current_model->bindPoseMatrices;
        model_data.bind_pose_matrices_original = model_data.bind_pose_matrices;
        
        // Initialize bone indices
        model_data.left_arm_indices = {6, 7, 17};
        model_data.right_arm_indices = {9, 10, 18};
        model_data.left_leg_indices = {11, 12, 13};
        model_data.right_leg_indices = {14, 15, 16};
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load model: " << e.what() << std::endl;
        return false;
    }
}

const InteractorModelData& get_interactor_model_data() {
    return model_data;
}

InteractorModel* get_interactor_model() {
    return current_model;
}

void update_bone_transforms(const std::vector<unsigned short>& indices) {
    for (short i = 0; i < indices.size() - 1; i++) {
        glm::vec3 translateVector = (model_data.bind_pose_positions[indices[i]] - 
                                   model_data.bind_pose_positions_original[indices[i]]);
        
        glm::vec3 original_dir = model_data.bind_pose_positions_original[indices[i + 1]] - 
                                model_data.bind_pose_positions_original[indices[i]];
        glm::vec3 current_dir = model_data.bind_pose_positions[indices[i + 1]] - 
                               model_data.bind_pose_positions[indices[i]];
        
        glm::vec3 rotationOrientationVector = glm::normalize(glm::cross(original_dir, current_dir));
        float rotationCos = glm::dot(glm::normalize(current_dir), glm::normalize(original_dir));
        
        // Clamp to avoid numerical issues
        rotationCos = glm::clamp(rotationCos, -1.0f, 1.0f);
        
        float rotationAngle = glm::acos(rotationCos);
        
        // Handle near-zero rotation
        if (glm::length(rotationOrientationVector) < 0.001f) {
            rotationOrientationVector = glm::vec3(0, 0, 1);
            rotationAngle = 0;
        }
        
        glm::quat rotationQuat = glm::angleAxis(rotationAngle, rotationOrientationVector);
        glm::mat4 offsetMatrix = glm::translate(glm::mat4(1.0f), 
                                              -model_data.bind_pose_positions_original[indices[i]]);
        
        model_data.bind_pose_matrices[indices[i]] = 
            glm::translate(glm::mat4(1.0f), translateVector) * 
            glm::inverse(offsetMatrix) * 
            glm::mat4_cast(rotationQuat) * 
            offsetMatrix;
        
        // Handle the last bone in the chain
        if (i == indices.size() - 2) {
            translateVector = (model_data.bind_pose_positions[indices[i + 1]] - 
                             model_data.bind_pose_positions_original[indices[i + 1]]);
            offsetMatrix = glm::translate(glm::mat4(1.0f), 
                                        -model_data.bind_pose_positions_original[indices[i + 1]]);
            
            model_data.bind_pose_matrices[indices[i + 1]] = 
                glm::translate(glm::mat4(1.0f), translateVector) * 
                glm::inverse(offsetMatrix) * 
                glm::mat4_cast(rotationQuat) * 
                offsetMatrix;
        }
    }
}

void sync_data_to_main(std::vector<glm::vec3>& main_positions, std::vector<glm::mat4>& main_matrices) {
    main_positions = model_data.bind_pose_positions;
    main_matrices = model_data.bind_pose_matrices;
}

void sync_data_from_main(const std::vector<glm::vec3>& main_positions, const std::vector<glm::mat4>& main_matrices) {
    model_data.bind_pose_positions = main_positions;
    model_data.bind_pose_matrices = main_matrices;
}
