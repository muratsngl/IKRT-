#ifndef MODEL_LOADER_HPP
#define MODEL_LOADER_HPP

#include <vector>
#include <glm/glm.hpp>
#include "assimp/scene.h"

// Forward declaration

class Shader;
class InteractorModel;
class SceneElementModel;

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
};

struct SceneElementModelData{
    glm::mat3 model_matrix;
    //add orientation and stuff
};

// Function declarations
bool load_scene_element_model(const char* path);
bool load_interactor_model(const char* path);
const InteractorModelData& get_interactor_model_data();
InteractorModel* get_interactor_model(); // Get the actual model for drawing
SceneElementModel* get_scene_element_model();
void update_bone_transforms(const std::vector<unsigned short>& indices);
void sync_data_to_main(std::vector<glm::vec3>& main_positions, std::vector<glm::mat4>& main_matrices);
void sync_data_from_main(const std::vector<glm::vec3>& main_positions, const std::vector<glm::mat4>& main_matrices);

#endif
