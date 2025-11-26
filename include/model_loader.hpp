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

// Bone manipulation mode
enum BoneManipulationMode {
    MODE_FORWARD_KINEMATICS,  // Children inherit parent transforms
    MODE_INVERSE_KINEMATICS   // Independent bone matrices for IK solving
};

// Model data structure
struct InteractorModelData {
    std::vector<glm::vec3> bind_pose_positions;
    std::vector<glm::vec3> bind_pose_positions_original;
    std::vector<glm::mat4> bind_pose_matrices;          // Final world-space matrices
    std::vector<glm::mat4> bind_pose_matrices_original;
    std::vector<glm::mat4> offset_matrices;       //offset matrices for origin based transformations in FK workflow removed the fuckface local transform logic
    std::vector<glm::mat4> imm_transformation_matrices;    //these hold t_current before offsetting
    BoneManipulationMode manipulation_mode = MODE_FORWARD_KINEMATICS;
};

struct SceneElementModelData{
    glm::mat3 model_matrix;
    //add orientation and stuff
};

// Function declarations
bool load_scene_element_model(const char* path);
bool load_interactor_model(const char* path);
bool load_interactable_model(const char* path);
bool remove_scene_element_model_by_id(int model_id);
void cleanup_scene_element_gpu_resources(SceneElementModel& model);
const InteractorModelData& get_interactor_model_data();
InteractorModelData& get_interactor_model_data_mutable();
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

void sync_data_from_main(const std::vector<glm::vec3>& main_positions, const std::vector<glm::mat4>& main_matrices);

// Helper function to check if interactor model is available
bool is_interactor_model_available();

#endif
