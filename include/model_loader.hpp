#ifndef MODEL_LOADER_HPP
#define MODEL_LOADER_HPP

#include <vector>
#include <glm/glm.hpp>

// Forward declaration
class Model;
class Shader;
class InteractorModel;

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

// Function declarations
bool load_interactor_model(const char* path);
const InteractorModelData& get_model_data();
InteractorModel* get_model(); // Get the actual model for drawing
void update_bone_transforms(const std::vector<unsigned short>& indices);
void sync_data_to_main(std::vector<glm::vec3>& main_positions, std::vector<glm::mat4>& main_matrices);
void sync_data_from_main(const std::vector<glm::vec3>& main_positions, const std::vector<glm::mat4>& main_matrices);

#endif
