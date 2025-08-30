#include "include/model_loader.hpp"
#include "include/model_bones.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>

static InteractorModelData model_data;
static InteractorModel* current_model = nullptr;
static std::vector<SceneElementModel> scene_element_model_arr;
//static std::vector<InteractableModel> interactable_model_arr;
static uint SceneElementModelCount = 0;

bool load_interactor_model(const char* path) {
    try {
        current_model = new InteractorModel(path);
        
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
        
        // Center bones (non-extremity bones: hip, spine, chest, neck, head, clavicles)
        model_data.center_bones = {0, 1, 2, 3, 4, 5, 8};

    model_data.left_thumb_indices = {17, 19, 20, 21, 49};

// Path: hand.L -> ... -> hand.L.013 -> hand.L.016
model_data.left_index_indices = {17, 22, 23, 24, 50};

// Path: hand.L -> ... -> hand.L.014 -> hand.L.018
model_data.left_middle_indices = {17, 25, 26, 27, 51};

// Path: hand.L -> ... -> hand.L.015 -> hand.L.019
model_data.left_ring_indices = {17, 28, 29, 30, 52};

// Path: hand.L -> ... -> hand.L.017 -> hand.L.020
model_data.left_pinky_indices = {17, 31, 32, 33, 53};


// --- Bone Index Paths for the Right Hand (UPDATED with Leaf Bones) ---
// The hierarchy is traced from the root bone hand.R (index 18).

// Path: hand.R -> ... -> hand.R.011 -> hand.R.012
model_data.right_thumb_indices = {18, 34, 35, 36, 54};

// Path: hand.R -> ... -> hand.R.013 -> hand.R.016
model_data.right_index_indices = {18, 37, 38, 39, 55};

// Path: hand.R -> ... -> hand.R.014 -> hand.R.018
model_data.right_middle_indices = {18, 40, 41, 42, 56};

// Path: hand.R -> ... -> hand.R.015 -> hand.R.019
model_data.right_ring_indices = {18, 43, 44, 45, 57};

// Path: hand.R -> ... -> hand.R.017 -> hand.R.020
model_data.right_pinky_indices = {18, 46, 47, 48, 58};


    return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load model: " << e.what() << std::endl;
        return false;
    }
}
bool load_scene_element_model(const char* path) {
    try {
        SceneElementModel* newModel = new SceneElementModel(path);
        scene_element_model_arr.push_back(*newModel);
        SceneElementModelCount++;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading SceneElementModel: " << e.what() << std::endl;
        return false;
    }
}

const InteractorModelData& get_interactor_model_data() {
    return model_data;
}

InteractorModel* get_interactor_model() {
    return current_model;
}

const SceneElementModel&get_scene_element_model(size_t index) {
    return scene_element_model_arr[index];
}
size_t get_scene_element_model_count() {
    return SceneElementModelCount;
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

void update_center_bone_matrices() {
    // Update transforms for center bones based on their bind pose positions
    for (unsigned short boneIndex : model_data.center_bones) {
        glm::vec3 translateVector = (model_data.bind_pose_positions[boneIndex] - 
                                   model_data.bind_pose_positions_original[boneIndex]);
        
        glm::mat4 offsetMatrix = glm::translate(glm::mat4(1.0f), 
                                              -model_data.bind_pose_positions_original[boneIndex]);
        
        model_data.bind_pose_matrices[boneIndex] = 
            glm::translate(glm::mat4(1.0f), translateVector) * 
            glm::inverse(offsetMatrix) * 
            offsetMatrix;
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

void end_effector_align(std::vector<unsigned short> indices) {
    if (indices.empty()) return;
    
    // Get the transformation matrix of the first index
    glm::mat4 firstTransform = model_data.bind_pose_matrices[indices[0]];
    
    // Assign this transformation matrix to all other indices
    for (size_t i = 1; i < indices.size(); i++) {
        model_data.bind_pose_matrices[indices[i]] = firstTransform;
    }
}

void apply_root_offset_to_bones(const glm::vec3& rootOffset) {
    // Apply root offset to all bone positions
    for (size_t i = 0; i < model_data.bind_pose_positions.size(); i++) {
        model_data.bind_pose_positions[i] +=rootOffset;
    }
}
