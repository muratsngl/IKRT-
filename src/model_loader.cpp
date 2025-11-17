#include "include/model_loader.hpp"
#include "include/model_bones.h"
#include "include/render_setup.hpp"
#include "include/application_logic.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>

static InteractorModelData model_data;
static InteractorModel* current_model = nullptr;
static std::vector<SceneElementModel> scene_element_model_arr;
static std::vector<InteractableModel> interactable_model_arr;
static uint SceneElementModelCount = 0;
static uint InteractableModelCount = 0;

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

        // Reset application state for the new model to ensure clean initialization
        reset_application_state();

    return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to load model: " << e.what() << std::endl;
        return false;
    }
}



//TODO::WRITE REMOVAL AND GARBAGE COLLECTION LOGIC
bool load_scene_element_model(const char* path) {
    try {
        SceneElementModel* newModel = new SceneElementModel(path);
        
        // Assign model index using the shared UBO index system
        newModel->model_index = get_model_index();
        
        // Register the model ID to index mapping
        register_model_id_to_index(newModel->id, newModel->model_index);
        
        scene_element_model_arr.push_back(*newModel);
        std::cout << "Successfully loaded SceneElementModel: " << path << " with ID: " << newModel->id << " and model index: " << newModel->model_index << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading SceneElementModel: " << e.what() << std::endl;
        return false;
    }
}

bool load_interactable_model(const char* path) {
    try {
        InteractableModel* newModel = new InteractableModel(path);
        
        // Assign model index using the shared UBO index system
        newModel->model_index = get_model_index();
        
        // Register the model ID to index mapping
        register_model_id_to_index(newModel->id, newModel->model_index);
        
        interactable_model_arr.push_back(*newModel);
        InteractableModelCount++;
        std::cout << "Successfully loaded InteractableModel: " << path << " with ID: " << newModel->id << " and model index: " << newModel->model_index << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading InteractableModel: " << e.what() << std::endl;
        return false;
    }
}

const InteractorModelData& get_interactor_model_data() {
    return model_data;
}

InteractorModel* get_interactor_model() {
    return current_model;
}

bool is_interactor_model_available() {
    return current_model != nullptr;
}

const SceneElementModel&get_scene_element_model(size_t index) {
    return scene_element_model_arr[index];
}

const InteractableModel& get_interactable_model(size_t index) {
    return interactable_model_arr[index];
}

size_t get_scene_element_model_count() {
    return scene_element_model_arr.size();
}

size_t get_interactable_model_count() {
    return InteractableModelCount;
}

std::vector<Shape>& get_interactable_element_boxes() {
    return interactable_element_boxes;
}

void update_bone_transforms(const std::vector<unsigned short>& indices) {
    for (unsigned short i = 0; i < indices.size() - 1; i++) {
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
        model_data.bind_pose_positions[i] += rootOffset;
    }
}

// Scene element model removal using swap-and-pop method
bool remove_scene_element_model_by_id(int model_id) {
    // Step 1: Find the model in the array by ID
    size_t model_array_index = static_cast<size_t>(-1);
    for (size_t i = 0; i < scene_element_model_arr.size(); i++) {
        if (scene_element_model_arr[i].id == model_id) {
            model_array_index = i;
            break;
        }
    }
    
    if (model_array_index == static_cast<size_t>(-1)) {
        std::cerr << "Error: Scene element model with ID " << model_id << " not found" << std::endl;
        return false;
    }
    
    SceneElementModel& model_to_remove = scene_element_model_arr[model_array_index];
    unsigned int removed_model_index = model_to_remove.model_index;
    
    // Step 2: Clean up GPU resources (VAO, VBO, EBO, textures)
    cleanup_scene_element_gpu_resources(model_to_remove);
    
    // Step 3: Remove collision boxes associated with this model
    extern std::vector<Shape> scene_element_boxes;
    scene_element_boxes.erase(
        std::remove_if(scene_element_boxes.begin(), scene_element_boxes.end(),
            [model_id](const Shape& shape) { return shape.id == model_id; }),
        scene_element_boxes.end()
    );
    
    // Step 4: If the selected model is being removed, clear selection
    extern void set_selected_object(int model_id);
    extern int get_selected_object_id();
    if (get_selected_object_id() == model_id) {
        set_selected_object(-1);
    }
    
    // Step 5: Swap with last element and pop (if not already last)
    size_t last_index = scene_element_model_arr.size() - 1;
    if (model_array_index != last_index) {
        // Swap the model to remove with the last model
        std::swap(scene_element_model_arr[model_array_index], scene_element_model_arr[last_index]);
        
        // The swapped model (now at model_array_index) keeps its model_index
        // No reindexing of UBO slot needed, just update the ID map is still valid
    }
    
    // Step 6: Pop the last element (which is now the model we want to remove)
    scene_element_model_arr.pop_back();
    
    // Step 7: Remove from ID→Index map
    extern void unregister_model_id_from_index(int model_id);
    unregister_model_id_from_index(model_id);
    
    std::cout << "Successfully removed scene element model ID: " << model_id << std::endl;
    return true;
}

// Helper function to clean up GPU resources for scene element model
void cleanup_scene_element_gpu_resources(SceneElementModel& model) {
    // Clean up each mesh's GPU resources
    for (auto& mesh : model.meshes) {
        mesh.cleanup();
    }
    
    // Delete textures
    // Note: In a more advanced implementation, you would want reference counting
    // for textures shared between models. For now, we delete them all.
    for (auto& texture : model.textures_loaded) {
        glDeleteTextures(1, &texture.id);
    }
}

// Helper functions for interactable models
