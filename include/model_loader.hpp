#ifndef MODEL_LOADER_HPP
#define MODEL_LOADER_HPP

#include <vector>
#include <glm/glm.hpp>
#include "assimp/scene.h"
#include "include/collision.hpp"

// Forward declaration

class Shader;
class InteractorModel;
class SceneElementModel;
class InteractableModel;

// Model data structure
struct InteractorModelData {
    std::vector<glm::vec3> bind_pose_positions;
    std::vector<glm::vec3> bind_pose_positions_original;
    std::vector<glm::mat4> bind_pose_matrices;
    std::vector<glm::mat4> bind_pose_matrices_original;
    
    // Bone indices for different limbs
    std::vector<unsigned short> left_arm_indices;
    std::vector<unsigned short> right_arm_indices;
    std::vector<unsigned short> left_leg_indices;
    std::vector<unsigned short> right_leg_indices;
    std::vector<unsigned short> left_thumb_indices;
    std::vector<unsigned short> right_thumb_indices;
    std::vector<unsigned short> left_index_indices;
    std::vector<unsigned short> right_index_indices;
    std::vector<unsigned short> left_middle_indices;
    std::vector<unsigned short> right_middle_indices;
    std::vector<unsigned short> left_ring_indices;
    std::vector<unsigned short> right_ring_indices;
    std::vector<unsigned short> left_pinky_indices;
    std::vector<unsigned short> right_pinky_indices;
    
    // Center bones (non-extremity bones: torso, spine, neck, head)
    std::vector<unsigned short> center_bones;

};

struct SceneElementModelData{
    glm::mat3 model_matrix;
    //add orientation and stuff
};

// Function declarations
bool load_scene_element_model(const char* path);
bool load_interactor_model(const char* path);
bool load_interactable_model(const char* path);
const InteractorModelData& get_interactor_model_data();
InteractorModel* get_interactor_model(); // Get the actual model for drawing
const SceneElementModel& get_scene_element_model(size_t index);
const InteractableModel& get_interactable_model(size_t index);
size_t get_scene_element_model_count();
size_t get_interactable_model_count();
std::vector<Shape>& get_interactable_element_boxes();
void update_bone_transforms(const std::vector<unsigned short>& indices);
void update_center_bone_matrices();
void end_effector_align(std::vector<unsigned short> indices);
void apply_root_offset_to_bones(const glm::vec3& rootOffset);
void sync_data_to_main(std::vector<glm::vec3>& main_positions, std::vector<glm::mat4>& main_matrices);

// Helper functions for interactable models
void draw_all_interactable_models(Shader& shader);
void update_all_interactable_model_bounding_boxes(const std::vector<glm::mat4>& boneTransforms);
void sync_data_from_main(const std::vector<glm::vec3>& main_positions, const std::vector<glm::mat4>& main_matrices);

#endif
